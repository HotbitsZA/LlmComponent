#pragma once
#include "iLlmProvider.hpp"
#include "cHTTPClient.h"
#include <string>

class cSessionMemory; // Forward declaration matching iLlmProvider interface requirements

class cOllamaProvider : public iLlmProvider
{
public:
    cOllamaProvider(std::string endpoint_url, std::string model_name);
    ~cOllamaProvider() override = default;

    [[nodiscard]] std::string query(const std::string &prompt,
                                    cSessionMemory *memory,
                                    const std::string& image_path = "") const override;
    [[nodiscard]] const std::string &name() const noexcept override;

private:
    std::string m_endpointUrl;
    std::string m_modelName;
    cHTTPClient m_httpClient;
};
