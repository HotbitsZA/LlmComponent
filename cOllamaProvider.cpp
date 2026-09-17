#include "cOllamaProvider.h"
#include "cSessionMemory.hpp"
#include <drogon/utils/Utilities.h> // Pulls in drogon::utils::base64Encode natively
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>

cOllamaProvider::cOllamaProvider(std::string endpoint_url, std::string model_name)
    : m_modelName(std::move(model_name))
{
    if (!endpoint_url.empty() && endpoint_url.back() == '/')
    {
        endpoint_url.pop_back();
    }
    size_t apiPos = endpoint_url.find("/api/");
    if (apiPos == std::string::npos)
    {
        m_endpointUrl = endpoint_url + "/api/chat";
    }
    else
    {
        m_endpointUrl = endpoint_url.substr(0, apiPos) + "/api/chat";
    }
}

const std::string &cOllamaProvider::name() const noexcept
{
    return m_modelName;
}

std::string cOllamaProvider::query(const std::string &prompt, cSessionMemory *memory, const std::string &image_path) const
{
    Json::Value root;
    root["model"] = m_modelName;
    root["stream"] = false;

    Json::Value messagesArray = Json::arrayValue;

    // 1. SYSTEM ROLE INJECTION
    Json::Value systemObj;
    systemObj["role"] = "system";

    // if (prompt.find("System Prompt Directive:") != std::string::npos)
    // {
    //     systemObj["content"] = prompt;
    // }
    // else
    // {
    //     systemObj["content"] = "You are a professional South African educator teaching the CAPS high school syllabus. "
    //                            "Be concise, highly clear, and follow lesson states strictly.";
    // }

    // Qwen 3B works best when the core identity remains unchanged at the top of the context window
    std::string baseInstruction = "You are Teacher Emma, an expert high school educator teaching the South African CAPS syllabus. "
                                  "Be brief, precise, use clear school level principles, and always format math in valid LaTeX.";

    if (prompt.find("System Prompt Directive:") != std::string::npos)
    {
        // Append the active lesson state instructions cleanly right to the core identity context
        systemObj["content"] = baseInstruction + " " + prompt;
    }
    else
    {
        systemObj["content"] = baseInstruction;
    }

    messagesArray.append(systemObj);

    // 2. CONVERSATIONAL TIMELINE PACKAGING (PAST HISTORY MOVED UP)
    if (memory != nullptr)
    {
        Json::Value historyPayload = memory->get_chat_array_payload();
        for (const auto &msg : historyPayload)
        {
            if (msg["role"].asString() != "system")
            {
                messagesArray.append(msg);
            }
        }
    }

    // 3. CURRENT USER PACKET & NATIVE DROGON BASE64 ENCODING HOOK
    Json::Value userObj;
    userObj["role"] = "user";
    userObj["content"] = prompt.find("System Prompt Directive:") != std::string::npos ? "" : prompt;

    // MULTIMODAL CHECK: If a valid file path was forwarded, encode it instantly!
    if (!image_path.empty())
    {
        std::ifstream file(image_path, std::ios::binary);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();

            std::string rawBinaryBytes = buffer.str();

            // ELITE DROGON CORE UTILITY CALL: Transforms image bytes into base64 instantly
            std::string base64String = drogon::utils::base64Encode(
                reinterpret_cast<const unsigned char *>(rawBinaryBytes.data()),
                rawBinaryBytes.length());

            // Append to Ollama's official vision array block spec
            Json::Value imagesArray = Json::arrayValue;
            imagesArray.append(base64String);
            userObj["images"] = imagesArray;

            std::cout << "[FundisaAI Vision] Encoded file: " << image_path
                      << " using drogon::utils (Size: " << base64String.length() << " characters)" << std::endl;
        }
        else
        {
            std::cerr << "[FundisaAI Vision] Warning: Unable to open target image file for base64 tracking: " << image_path << std::endl;
        }
    }

    // Append the current fresh user payload frame into the active execution sequence loop
    bool hasContent = !userObj["content"].asString().empty() || userObj.isMember("images");
    if (hasContent)
    {
        messagesArray.append(userObj);
    }

    root["messages"] = messagesArray;

    Json::StreamWriterBuilder writer;
    const std::string jsonRequestString = Json::writeString(writer, root);

    // 4. EXECUTE NETWORK TRANSACTION via your verified repository cHTTPClient
    auto httpResponse = m_httpClient.post(m_endpointUrl, jsonRequestString, {}, "application/json");
    if (!httpResponse.ok())
    {
        std::cerr << "[cOllamaProvider] Connection failed: HTTP Status " << httpResponse.statusCode << std::endl;
        return "I am having a brief problem accessing our classroom textbook notes right now, learner.";
    }

    Json::Value responseRoot;
    Json::CharReaderBuilder reader;
    std::string parseErrors;
    std::stringstream responseStream(httpResponse.body);

    if (!Json::parseFromStream(reader, responseStream, &responseRoot, &parseErrors))
    {
        return "I couldn't quite read that page of our lesson material, student.";
    }

    if (responseRoot.isMember("message") && responseRoot["message"].isMember("content"))
    {
        std::string assistantReply = responseRoot["message"]["content"].asString();

        // 5. UPDATE HISTORICAL PERSISTENCE MATRIX SAFELY AFTER SUCCESSFUL INFERENCE MATCH
        if (memory != nullptr)
        {
            // Only remember actual user conversational inputs, skip backend system macro modifications
            if (prompt.find("System Prompt Directive:") == std::string::npos && !prompt.empty())
            {
                memory->append_user_message(prompt);
            }
            memory->append_assistant_message(assistantReply);
        }
        return assistantReply;
    }

    return "[Teacher response data format is missing or invalid]";
}
