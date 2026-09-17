#pragma once
#include <json/json.h>
#include <string>
#include <mutex>

class cSessionMemory
{
public:
    explicit cSessionMemory(size_t max_history_turns = 10)
        : m_maxHistoryTurns(max_history_turns)
    {
        // Establish initial permanent background system guidelines
        m_systemMessage["role"] = "system";
        m_systemMessage["content"] = "You are a concise voice assistant. Respond in one or two short sentences max. "
                                     "Do not include markdown, stars, lists, or headers.";
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

    [[nodiscard]] Json::Value get_chat_array_payload() const
    {
        std::lock_guard<std::mutex> lock(m_memoryMutex);
        Json::Value payload = Json::arrayValue;
        payload.append(m_systemMessage);

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
    Json::Value m_systemMessage;
    Json::Value m_history{Json::arrayValue};
    mutable std::mutex m_memoryMutex;
};
