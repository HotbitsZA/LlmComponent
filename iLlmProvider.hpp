#pragma once
#include <string>

class cSessionMemory; // Forward declaration

class iLlmProvider
{
public:
    virtual ~iLlmProvider() = default;

    /**
     * @brief Synchronously processes a prompt text line with chat history tracking capabilities.
     */
    [[nodiscard]] virtual std::string query(const std::string &prompt, cSessionMemory *memory, const std::string& image_path = "") const = 0;

    [[nodiscard]] virtual const std::string &name() const noexcept = 0;
};
