#pragma once

#include "cBaseWorker_V2.h" // Resolved cleanly via CMake include search paths
#include "iLlmProvider.hpp"
#include "cSessionMemory.hpp"
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

    cLlmBrainWorker(std::shared_ptr<iLlmProvider> provider, LlmResponseCallback callback = nullptr);
    ~cLlmBrainWorker() noexcept override;

    void request_response(const std::string &userPrompt);
    void set_callback(LlmResponseCallback callback);

protected:
    bool preRun() override;
    void run() override;
    void stopTriggered() override;

private:
    std::shared_ptr<iLlmProvider> m_provider;
    LlmResponseCallback m_onResponseGenerated;

    std::queue<std::string> m_promptQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;

    cSessionMemory m_sessionMemory; // Now a fully complete layout type
};
