#include "base64_encoder.hpp"
#include "cSessionMemory.hpp"
#include "llm_config.hpp"
#include "llm_payload.hpp"

#include <json/json.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do                                                                    \
    {                                                                     \
        if (!(cond))                                                      \
        {                                                                 \
            std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")" \
                      << std::endl;                                       \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

static void test_base64_vectors()
{
    CHECK(base64_encode("") == "");
    CHECK(base64_encode("f") == "Zg==");
    CHECK(base64_encode("fo") == "Zm8=");
    CHECK(base64_encode("foo") == "Zm9v");
    CHECK(base64_encode("foob") == "Zm9vYg==");
    CHECK(base64_encode("fooba") == "Zm9vYmE=");
    CHECK(base64_encode("foobar") == "Zm9vYmFy");

    const std::uint8_t binary[] = {0x00, 0xFF, 0x80, 0x01};
    CHECK(base64_encode(binary, 4) == "AP+AAQ==");
}

static void test_directive_protocol()
{
    CHECK(!is_system_directive_prompt("hello there"));
    CHECK(is_system_directive_prompt("System Prompt Directive: keep it short"));
    CHECK(is_system_directive_prompt("Lesson state: System Prompt Directive: x"));

    const std::string base = "You are base.";
    CHECK(augment_system_prompt("hello", base) == base);
    CHECK(augment_system_prompt("System Prompt Directive: stay formal", base) ==
          base + " System Prompt Directive: stay formal");
}

static void test_build_chat_request_basic()
{
    LlmConfig cfg;
    cfg.model = "llama3.2";
    cfg.systemPrompt = "You are a test assistant.";
    cfg.temperature = 0.8f;
    cfg.numPredict = 128;

    Json::Value history = Json::arrayValue;
    Json::Value past;
    past["role"] = "user";
    past["content"] = "earlier question";
    history.append(past);
    Json::Value systemInHistory;
    systemInHistory["role"] = "system";
    systemInHistory["content"] = "should be skipped";
    history.append(systemInHistory);

    const Json::Value req = build_chat_request(cfg, "What is 2+2?", "", history);

    CHECK(req["model"].asString() == "llama3.2");
    CHECK(req["stream"].asBool() == false);
    CHECK(req["options"]["temperature"].asFloat() > 0.79f);
    CHECK(req["options"]["num_predict"].asInt() == 128);

    const Json::Value &msgs = req["messages"];
    CHECK(msgs.isArray());

    // system first, from config
    CHECK(msgs[0]["role"].asString() == "system");
    CHECK(msgs[0]["content"].asString() == "You are a test assistant.");

    // history appended, minus the system role
    CHECK(msgs.size() == 3u);
    CHECK(msgs[1]["role"].asString() == "user");
    CHECK(msgs[1]["content"].asString() == "earlier question");

    // current user message last
    CHECK(msgs[2]["role"].asString() == "user");
    CHECK(msgs[2]["content"].asString() == "What is 2+2?");
    CHECK(!msgs[2].isMember("images"));
}

static void test_build_chat_request_directive_and_vision()
{
    LlmConfig cfg;
    cfg.systemPrompt = "Base prompt.";

    // Directive: augmented into system, no user content message.
    const Json::Value reqDirective =
        build_chat_request(cfg, "System Prompt Directive: focus on algebra", "", Json::Value(Json::arrayValue));
    const Json::Value &msgsD = reqDirective["messages"];
    CHECK(msgsD[0]["content"].asString() ==
          "Base prompt. System Prompt Directive: focus on algebra");
    CHECK(msgsD.size() == 1u);

    // Vision: images array attached to the user message.
    const Json::Value reqVision =
        build_chat_request(cfg, "describe this", "QUJDRA==", Json::Value(Json::arrayValue));
    const Json::Value &msgsV = reqVision["messages"];
    CHECK(msgsV.size() == 2u);
    CHECK(msgsV[1]["content"].asString() == "describe this");
    CHECK(msgsV[1]["images"].isArray());
    CHECK(msgsV[1]["images"][0].asString() == "QUJDRA==");

    // Directive + vision: no content, images still attached.
    const Json::Value reqDV =
        build_chat_request(cfg, "System Prompt Directive: analyze image", "QUJDRA==",
                           Json::Value(Json::arrayValue));
    const Json::Value &msgsDV = reqDV["messages"];
    CHECK(msgsDV.size() == 2u);
    CHECK(msgsDV[1]["content"].asString().empty());
    CHECK(msgsDV[1]["images"][0].asString() == "QUJDRA==");
}

static void test_extract_assistant_reply()
{
    std::string out;

    Json::Value valid;
    valid["message"]["role"] = "assistant";
    valid["message"]["content"] = "The answer is 42.";
    CHECK(extract_assistant_reply(valid, out));
    CHECK(out == "The answer is 42.");

    Json::Value noMessage;
    CHECK(!extract_assistant_reply(noMessage, out));

    Json::Value noContent;
    noContent["message"]["role"] = "assistant";
    CHECK(!extract_assistant_reply(noContent, out));

    Json::Value nonString;
    CHECK(!extract_assistant_reply(nonString, out));

    Json::Value bad;
    bad["message"]["content"] = Json::Value(Json::nullValue);
    CHECK(!extract_assistant_reply(bad, out));

    Json::Value notObject;
    notObject = Json::Value(Json::arrayValue);
    CHECK(!extract_assistant_reply(notObject, out));
}

static void test_session_memory_pruning()
{
    cSessionMemory mem(2); // max 2 turns => 4 messages

    // Fresh memory holds no injected system message.
    CHECK(mem.get_history_payload().size() == 0u);

    for (int i = 1; i <= 3; ++i)
    {
        mem.append_user_message("user" + std::to_string(i));
        mem.append_assistant_message("assistant" + std::to_string(i));
    }

    const Json::Value history = mem.get_history_payload();
    CHECK(history.size() == 4u); // 6 messages minus oldest turn
    CHECK(history[0]["role"].asString() == "user");
    CHECK(history[0]["content"].asString() == "user2");
    CHECK(history[3]["role"].asString() == "assistant");
    CHECK(history[3]["content"].asString() == "assistant3");

    // No system role ever stored.
    for (const auto &msg : history)
    {
        CHECK(msg["role"].asString() != "system");
    }

    mem.clear();
    CHECK(mem.get_history_payload().size() == 0u);
}

static void test_base64_image_file_missing()
{
    CHECK(base64_image_file("/nonexistent/path/file.png").empty());
    CHECK(base64_image_file("").empty());
}

int main()
{
    test_base64_vectors();
    test_directive_protocol();
    test_build_chat_request_basic();
    test_build_chat_request_directive_and_vision();
    test_extract_assistant_reply();
    test_session_memory_pruning();
    test_base64_image_file_missing();

    if (g_failures == 0)
    {
        std::cout << "All tests passed." << std::endl;
        return 0;
    }
    std::cerr << g_failures << " test(s) FAILED" << std::endl;
    return 1;
}