#include "cOllamaProvider.h"
#include "cSessionMemory.hpp"
#include "llm_payload.hpp"
#include <json/json.h>
#include <stdexcept>
#include <sstream>

cOllamaProvider::cOllamaProvider(const LlmConfig &cfg)
    : m_cfg(cfg)
{
    std::string endpoint = m_cfg.endpoint;
    if (!endpoint.empty() && endpoint.back() == '/')
    {
        endpoint.pop_back();
    }
    size_t apiPos = endpoint.find("/api/");
    if (apiPos == std::string::npos)
    {
        m_endpointUrl = endpoint + "/api/chat";
    }
    else
    {
        m_endpointUrl = endpoint.substr(0, apiPos) + "/api/chat";
    }
}

const std::string &cOllamaProvider::name() const noexcept
{
    return m_cfg.model;
}

std::string cOllamaProvider::query(const std::string &prompt, cSessionMemory *memory, const std::string &image_path) const
{
    Json::Value history = memory ? memory->get_history_payload() : Json::Value(Json::arrayValue);
    const std::string imageBase64 = base64_image_file(image_path);

    const Json::Value requestRoot = build_chat_request(m_cfg, prompt, imageBase64, history);

    Json::StreamWriterBuilder writer;
    const std::string jsonRequestString = Json::writeString(writer, requestRoot);

    auto httpResponse = m_httpClient.post(m_endpointUrl, jsonRequestString, {}, "application/json");
    if (!httpResponse.ok())
    {
        throw std::runtime_error(
            "cOllamaProvider: HTTP " + std::to_string(httpResponse.statusCode) +
            " (" + std::to_string(httpResponse.curlCode) + ") from " + m_endpointUrl +
            (httpResponse.errorMessage.empty() ? std::string() : ": " + httpResponse.errorMessage));
    }

    Json::Value responseRoot;
    Json::CharReaderBuilder reader;
    std::string parseErrors;
    std::istringstream responseStream(httpResponse.body);

    if (!Json::parseFromStream(reader, responseStream, &responseRoot, &parseErrors))
    {
        throw std::runtime_error("cOllamaProvider: failed to parse response body: " + parseErrors);
    }

    std::string assistantReply;
    if (!extract_assistant_reply(responseRoot, assistantReply))
    {
        throw std::runtime_error("cOllamaProvider: response payload has no message.content");
    }

    if (memory != nullptr)
    {
        // Only remember actual user conversational inputs, skip system directive macros.
        if (!is_system_directive_prompt(prompt) && !prompt.empty())
        {
            memory->append_user_message(prompt);
        }
        memory->append_assistant_message(assistantReply);
    }

    return assistantReply;
}