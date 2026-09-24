#include "cLlmBrainWorker.h"
#include <iostream>
#include <stdexcept>

cLlmBrainWorker::cLlmBrainWorker(std::shared_ptr<iLlmProvider> provider, const LlmConfig &cfg,
                                 LlmResponseCallback callback)
    : cBaseWorker_V2("LlmBrainWorker"),
      m_cfg(cfg),
      m_provider(std::move(provider)),
      m_onResponseGenerated(std::move(callback)),
      m_sessionMemory(m_cfg.maxHistoryTurns)
{
    if (!m_provider)
    {
        throw std::runtime_error("LlmBrainWorker Error: Injected LLM provider pointer is null!");
    }
}

cLlmBrainWorker::~cLlmBrainWorker() noexcept
{
    // Guarantee the worker thread (which uses m_provider/m_sessionMemory) has
    // fully exited before those members are destroyed.
    stopThreadAndJoin();
}

void cLlmBrainWorker::request_response(const std::string &userPrompt, const std::string &imagePath)
{
    if (userPrompt.empty())
    {
        return;
    }
    if (userPrompt.size() > m_cfg.maxPromptChars)
    {
        std::cerr << "[" << name() << "] WARNING: prompting input exceeds maxPromptChars, ignored "
                  << "(" << userPrompt.size() << " chars)" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);

    if (m_promptQueue.size() >= m_cfg.maxQueueDepth)
    {
        // Back-pressure: keep the newest prompt, drop the oldest, warn throttled.
        m_promptQueue.pop();
        ++m_dropped;
        if (m_dropped == 1 || (m_dropped % 32) == 0)
        {
            std::cerr << "[" << name() << "] WARNING: prompt backlog, dropped "
                      << m_dropped << " request(s)" << std::endl;
        }
    }

    m_promptQueue.push(Prompt{userPrompt, imagePath});
    m_queueCV.notify_one();
}

void cLlmBrainWorker::set_callback(LlmResponseCallback callback)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_onResponseGenerated = std::move(callback);
}

bool cLlmBrainWorker::preRun()
{
    std::cout << "[" << name() << "] LLM backend online: " << m_provider->name() << std::endl;
    return true;
}

void cLlmBrainWorker::run()
{
    while (continueRunning())
    {
        Prompt current;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this]()
                           { return !m_promptQueue.empty() || stopRequested(); });

            if (stopRequested() && m_promptQueue.empty())
            {
                break;
            }
            if (!m_promptQueue.empty())
            {
                current = std::move(m_promptQueue.front());
                m_promptQueue.pop();
            }
        }

        if (current.text.empty())
        {
            continue;
        }

        updateHeartbeat();

        std::string reply;
        try
        {
            reply = m_provider->query(current.text, &m_sessionMemory, current.imagePath);
        }
        catch (const std::exception &e)
        {
            std::cerr << "[" << name() << "] Query failure: " << e.what() << std::endl;
            continue;
        }

        LlmResponseCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            cb = m_onResponseGenerated;
        }

        if (cb && !reply.empty())
        {
            cb(reply);
        }
    }
}

void cLlmBrainWorker::stopTriggered()
{
    m_queueCV.notify_all();
}