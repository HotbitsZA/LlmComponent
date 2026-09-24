#pragma once

#include <cstdint>
#include <string>

// Central configuration for the LLM provider, its payload builder, and the
// async worker that dispatches prompts.
struct LlmConfig
{
    // Ollama HTTP endpoint, e.g. http://localhost:11434.
    std::string endpoint = "http://localhost:11434";

    // Model to query on the server.
    std::string model = "llama3.2";

    // Single source of truth for the system prompt. Kept as the default persona
    // so existing callers see unchanged behavior until they override it.
    std::string systemPrompt =
        "You are Teacher Emma, an expert high school educator teaching the "
        "South African CAPS syllabus. Be brief, precise, use clear school level "
        "principles, and always format math in valid LaTeX.";

    // Sampler overrides (0 = leave the server defaults untouched).
    float temperature = 0.0f;
    int numPredict = 0;

    // Transport timeouts (milliseconds).
    std::int32_t connectTimeoutMs = 10000;
    std::int32_t requestTimeoutMs = 180000;

    // Session memory: turns of (user + assistant) retained per session.
    std::size_t maxHistoryTurns = 10;

    // Async worker back-pressure.
    std::size_t maxQueueDepth = 32;
    std::size_t maxPromptChars = 65536;
};