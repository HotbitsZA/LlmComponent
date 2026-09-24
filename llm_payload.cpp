#include "llm_payload.hpp"

#include "base64_encoder.hpp"

#include <fstream>
#include <sstream>

bool is_system_directive_prompt(const std::string &utterance)
{
    return utterance.find(kSystemDirectiveMarker) != std::string::npos;
}

std::string augment_system_prompt(const std::string &utterance,
                                  const std::string &baseSystemPrompt)
{
    if (is_system_directive_prompt(utterance))
    {
        return baseSystemPrompt + " " + utterance;
    }
    return baseSystemPrompt;
}

Json::Value build_chat_request(const LlmConfig &cfg,
                               const std::string &utterance,
                               const std::string &imageBase64,
                               const Json::Value &historyMessages)
{
    Json::Value root;
    root["model"] = cfg.model;
    root["stream"] = false;

    if (cfg.temperature > 0.0f)
    {
        root["options"]["temperature"] = cfg.temperature;
    }
    if (cfg.numPredict > 0)
    {
        root["options"]["num_predict"] = cfg.numPredict;
    }

    const bool directive = is_system_directive_prompt(utterance);

    Json::Value messages = Json::arrayValue;

    Json::Value system;
    system["role"] = "system";
    system["content"] = augment_system_prompt(utterance, cfg.systemPrompt);
    messages.append(system);

    // Conversational history (subject only - never system roles).
    if (historyMessages.isArray())
    {
        for (const auto &msg : historyMessages)
        {
            if (msg["role"].asString() != "system")
            {
                messages.append(msg);
            }
        }
    }

    // Current user packet. Directive utterances carry no user content.
    Json::Value user;
    user["role"] = "user";
    user["content"] = directive ? std::string() : utterance;
    if (!imageBase64.empty())
    {
        Json::Value images = Json::arrayValue;
        images.append(imageBase64);
        user["images"] = images;
    }

    const bool hasContent = !user["content"].asString().empty() || user.isMember("images");
    if (hasContent)
    {
        messages.append(user);
    }

    root["messages"] = messages;
    return root;
}

bool extract_assistant_reply(const Json::Value &responseRoot, std::string &out)
{
    if (!responseRoot.isObject() || !responseRoot.isMember("message"))
    {
        return false;
    }
    const Json::Value &message = responseRoot["message"];
    if (!message.isObject() || !message.isMember("content") || !message["content"].isString())
    {
        return false;
    }
    out = message["content"].asString();
    return true;
}

std::string base64_image_file(const std::string &imagePath)
{
    if (imagePath.empty())
    {
        return std::string();
    }

    std::ifstream file(imagePath, std::ios::binary);
    if (!file.is_open())
    {
        return std::string();
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return base64_encode(buffer.str());
}