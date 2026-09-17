#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include "cLlmBrainWorker.h"
#include "cOllamaProvider.h"

// Synchronises console prints between the main thread and background worker callbacks
std::mutex g_logMutex;

// Asynchronous completion hook event fired directly from the worker thread
void onBrainResponseReceived(const std::string &reply)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::cout << "\n🤖 [LLM RESPONSE RECEIVED]:" << std::endl;
    std::cout << "💬 " << reply << std::endl;
    std::cout << "---------------------------------------------------------\n"
              << std::endl;
}

int main()
{
    std::cout << "Starting Type-Safe Modular LLM Pipeline Integration Test..." << std::endl;

    // Local target endpoint coordinates configuration properties
    // Assumes your background Ollama engine is running locally on port 11434
    //std::string ollama_endpoint = "http://localhost:11434/api/generate";
    std::string ollama_url = "http://localhost:11434/api/chat";
    std::string model_name = "llama3.2"; // Update this string to match any model you have pulled

    // 1. DEPENDENCY INJECTION: Construct the explicit backend provider asset layer
    auto localOllamaBackend = std::make_shared<cOllamaProvider>(ollama_url, model_name);

    // 2. Pass the concrete engine asset right into the constructor handle mapping
    std::unique_ptr<cLlmBrainWorker> brainWorker;
    try
    {
        brainWorker = std::make_unique<cLlmBrainWorker>(localOllamaBackend, onBrainResponseReceived);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Initialization Aborted: " << e.what() << std::endl;
        return 1;
    }

    // 3. Launch the background execution machine layer thread context
    if (!brainWorker->startThread())
    {
        std::cerr << "Fatal Error: Failed to start the Ollama background worker thread." << std::endl;
        return 1;
    }

    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "\n==========================================================" << std::endl;
        std::cout << "🧠 Local LLM Brain Worker successfully launched." << std::endl;
        std::cout << "Asynchronously dispatching two test requests to the queue..." << std::endl;
        std::cout << "==========================================================\n"
                  << std::endl;
    }

    // 4. Dispatch tasks over into the asynchronous processing pipelines
    brainWorker->request_response("Why is the sky blue? Answer in one short sentence.");
    brainWorker->request_response("What is the speed of light in meters per second? One short sentence.");

    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "👉 [Main Thread] Promotional queues loaded! Returning flow control instantly." << std::endl;
        std::cout << "Waiting for the backend network loop to fire completions out-of-band..." << std::endl;
    }

    // 5. Idle main thread to allow cURL perform steps to cross system network bounds completely
    for (int i = 0; i < 20; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "." << std::flush;
    }

    // 6. Clean cooperative closure sequence
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::cout << "\n\nShutting down brain layers cleanly..." << std::endl;
    }

    static_cast<void>(brainWorker->stopThread());

    // Explicit pointer pointer release structure inside scope boundaries
    brainWorker.reset();

    std::cout << "Application exited successfully." << std::endl;
    return 0;
}
