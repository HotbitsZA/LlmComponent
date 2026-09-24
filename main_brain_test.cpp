#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "cLlmBrainWorker.h"
#include "cOllamaProvider.h"
#include "llm_config.hpp"

namespace
{
    std::mutex g_logMutex;

    void onBrainResponseReceived(const std::string &reply)
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "\n[LLM RESPONSE RECEIVED]:" << std::endl;
        std::cout << "  " << reply << std::endl;
        std::cout << "---------------------------------------------------------\n"
                  << std::endl;
    }

    // argv -> env -> default
    std::string resolve_endpoint(int argc, char **argv)
    {
        if (argc > 1 && argv[1] != nullptr && argv[1][0] != '\0')
        {
            return argv[1];
        }
        if (const char *env = std::getenv("OLLAMA_ENDPOINT"))
        {
            if (env[0] != '\0')
            {
                return env;
            }
        }
        return "http://localhost:11434";
    }

    std::string resolve_model(int argc, char **argv)
    {
        if (argc > 2 && argv[2] != nullptr && argv[2][0] != '\0')
        {
            return argv[2];
        }
        if (const char *env = std::getenv("OLLAMA_MODEL"))
        {
            if (env[0] != '\0')
            {
                return env;
            }
        }
        return "llama3.2";
    }
} // namespace

int main(int argc, char **argv)
{
    LlmConfig cfg;
    cfg.endpoint = resolve_endpoint(argc, argv);
    cfg.model = resolve_model(argc, argv);

    std::cout << "Starting Type-Safe Modular LLM Pipeline Integration Test..." << std::endl;
    std::cout << "  Endpoint : " << cfg.endpoint << std::endl;
    std::cout << "  Model    : " << cfg.model << std::endl;

    // 1. Dependency injection: construct the concrete backend provider.
    auto localOllamaBackend = std::make_shared<cOllamaProvider>(cfg);

    // 2. Hand the concrete engine into the worker constructor.
    std::unique_ptr<cLlmBrainWorker> brainWorker;
    try
    {
        brainWorker = std::make_unique<cLlmBrainWorker>(localOllamaBackend, cfg, onBrainResponseReceived);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Initialization Failed: " << e.what() << std::endl;
        return 1;
    }

    // 3. Launch the background execution worker thread.
    if (!brainWorker->startThread())
    {
        std::cerr << "Fatal Error: Failed to start the LLM background worker thread." << std::endl;
        return 1;
    }

    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "\n==========================================================" << std::endl;
        std::cout << "Local LLM Brain Worker successfully launched." << std::endl;
        std::cout << "Asynchronously dispatching two test requests to the queue..." << std::endl;
        std::cout << "==========================================================\n"
                  << std::endl;
    }

    // 4. Dispatch tasks into the asynchronous processing pipeline.
    brainWorker->request_response("Why is the sky blue? Answer in one short sentence.");
    brainWorker->request_response("What is the speed of light in meters per second? One short sentence.");

    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "[Main Thread] Prompt queue loaded! Returning flow control instantly." << std::endl;
        std::cout << "Waiting for the backend network loop to fire completions..." << std::endl;
    }

    // 5. Idle the main thread to let the worker drain the queue.
    for (int i = 0; i < 20; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "." << std::flush;
    }

    // 6. Clean cooperative shutdown.
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "\n\nShutting down brain layers cleanly..." << std::endl;
    }

    brainWorker->stopThreadAndJoin();
    brainWorker.reset();

    std::cout << "Application exited successfully." << std::endl;
    return 0;
}