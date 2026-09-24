#pragma once
#include "iLlmProvider.hpp"
#include "llm_config.hpp"
#include "cHTTPClient.h"
#include <string>

class cSessionMemory; // Forward declaration matching iLlmProvider interface requirements

class cOllamaProvider : public iLlmProvider
{
public:
    explicit cOllamaProvider(const LlmConfig &cfg);
    ~cOllamaProvider() override = default;

    [[nodiscard]] std::string query(const std::string &prompt,
                                    cSessionMemory *memory,
                                    const std::string &image_path = "") const override;
    [[nodiscard]] const std::string &name() const noexcept override;

private:
    LlmConfig m_cfg;
    std::string m_endpointUrl;
    cHTTPClient m_httpClient;
};