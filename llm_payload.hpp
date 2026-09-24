#pragma once

#include "llm_config.hpp"

#include <json/json.h>

#include <string>

// Protocol marker embedded at the start of an utterance to swap that utterance
// into the system prompt (see augment_system_prompt).
inline constexpr const char *kSystemDirectiveMarker = "System Prompt Directive:";

// True when the utterance carries the system-directive protocol marker.
bool is_system_directive_prompt(const std::string &utterance);

// Effective system prompt: when the utterance begins with the directive marker,
// the whole utterance is appended to the base system prompt (preserving the
// original "lesson state" behavior); otherwise the base is returned unchanged.
std::string augment_system_prompt(const std::string &utterance,
                                  const std::string &baseSystemPrompt);

// Builds the Ollama /api/chat request body: system (from config, directive-aware),
// then conversational history (excluding any system roles), then the current user
// message. The user message carries `imageBase64` when vision is requested and is
// omitted entirely when it has neither content nor an image (directive case.
// Pure logic: no I/O, fully testable.
Json::Value build_chat_request(const LlmConfig &cfg,
                               const std::string &utterance,
                               const std::string &imageBase64,
                               const Json::Value &historyMessages);

// Extracts the assistant reply from an Ollama chat response. Returns false when
// the payload does not contain message.content (transport-level failures are
// reported before this is reached).
bool extract_assistant_reply(const Json::Value &responseRoot, std::string &out);

// Reads a file and returns its Base64 encoding. Returns an empty string when the
// path is empty or the file cannot be opened.
std::string base64_image_file(const std::string &imagePath);