#pragma once
#include <json/json.h>
#include <string>
#include <mutex>

// Thread-safe, bounded conversational history. The system prompt is owned by
// LlmConfig (single source of truth), so this store holds only user and
// assistant turns.
class cSessionMemory
{
public:
    explicit cSessionMemory(size_t max_history_turns = 10)
        : m_maxHistoryTurns(max_history_turns)
    {
    }

    void append_user_message(const std::string &text)
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);
        Json::Value msg;
        msg["role"] = "user";
        msg["content"] = text;
        m_history.append(msg);
        prune_history_if_needed();
    }

    void append_assistant_message(const std::string &text)
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);
        Json::Value msg;
        msg["role"] = "assistant";
        msg["content"] = text;
        m_history.append(msg);
        prune_history_if_needed();
    }

    // Conversational history only (no injected system message).
    [[nodiscard]] Json::Value get_history_payload() const
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);
        Json::Value payload = Json::arrayValue;
        for (const auto &msg : m_history)
        {
            payload.append(msg);
        }
        return payload;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);
        m_history.clear();
    }

private:
    void prune_history_if_needed()
    {
        // Each turn contains 2 messages: 1 user, 1 assistant.
        size_t maxMessages = m_maxHistoryTurns * 2;
        while (m_history.size() > maxMessages)
        {
            Json::Value newHistory = Json::arrayValue;
            // Erase oldest turn (first two elements) to preserve contextual cohesion
            for (Json::ArrayIndex i = 2; i < m_history.size(); ++i)
            {
                newHistory.append(m_history[i]);
            }
            m_history = std::move(newHistory);
        }
    }

    size_t m_maxHistoryTurns;
    Json::Value m_history{Json::arrayValue};
    mutable std::mutex m_memoryMutex;
};