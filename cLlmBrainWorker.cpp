#include "cLlmBrainWorker.h"
#include <iostream>

cLlmBrainWorker::cLlmBrainWorker(std::shared_ptr<iLlmProvider> provider, LlmResponseCallback callback)
    : cBaseWorker_V2("OllamaLlmBrainWorker"),
      m_provider(std::move(provider)),
      m_onResponseGenerated(std::move(callback))
{
    if (!m_provider)
    {
        throw std::runtime_error("LlmBrainWorker Error: Injected LLM provider pointer is null!");
    }
}

cLlmBrainWorker::~cLlmBrainWorker() noexcept
{
    static_cast<void>(stopThread());
}

void cLlmBrainWorker::request_response(const std::string &userPrompt)
{
    if (userPrompt.empty())
        return;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_promptQueue.push(userPrompt);
    }
    m_queueCV.notify_one();
}

void cLlmBrainWorker::set_callback(LlmResponseCallback callback)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_onResponseGenerated = std::move(callback);
}

bool cLlmBrainWorker::preRun()
{
    std::cout << "[" << name() << "] Injecting provider tracking: " << m_provider->name() << std::endl;
    return true;
}

void cLlmBrainWorker::run()
{
    while (continueRunning())
    {
        std::string currentPrompt;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this]()
                           { return !m_promptQueue.empty() || stopRequested(); });

            if (stopRequested() && m_promptQueue.empty())
                break;

            if (!m_promptQueue.empty())
            {
                currentPrompt = std::move(m_promptQueue.front());
                m_promptQueue.pop();
            }
        }

        if (!currentPrompt.empty())
        {
            updateHeartbeat();

            std::string generatedReply = m_provider->query(currentPrompt, &m_sessionMemory);

            LlmResponseCallback targetCallback;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                targetCallback = m_onResponseGenerated;
            }

            if (targetCallback && !generatedReply.empty())
            {
                targetCallback(generatedReply);
            }
        }
    }
}

void cLlmBrainWorker::stopTriggered()
{
    m_queueCV.notify_all();
}
