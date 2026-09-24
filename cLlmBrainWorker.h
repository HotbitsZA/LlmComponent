#pragma once

#include "cBaseWorker_V2.h"
#include "iLlmProvider.hpp"
#include "cSessionMemory.hpp"
#include "llm_config.hpp"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

class cLlmBrainWorker : public cBaseWorker_V2
{
public:
    using LlmResponseCallback = std::function<void(const std::string &replyText)>;

    cLlmBrainWorker(std::shared_ptr<iLlmProvider> provider, const LlmConfig &cfg,
                    LlmResponseCallback callback = nullptr);
    ~cLlmBrainWorker() noexcept override;

    void request_response(const std::string &userPrompt, const std::string &imagePath = "");
    void set_callback(LlmResponseCallback callback);

protected:
    bool preRun() override;
    void run() override;
    void stopTriggered() override;

private:
    struct Prompt
    {
        std::string text;
        std::string imagePath;
    };

    LlmConfig m_cfg;
    std::shared_ptr<iLlmProvider> m_provider;
    mutable std::mutex m_cbMutex;
    LlmResponseCallback m_onResponseGenerated;

    std::queue<Prompt> m_promptQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;
    size_t m_dropped = 0;

    cSessionMemory m_sessionMemory;
};