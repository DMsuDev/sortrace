// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Small parsing helpers for command lines and prompts.
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace sortrace::examples {

[[nodiscard]] inline bool parseInt(const std::string& text, long long& out)
{
    char* end = nullptr;
    out = std::strtoll(text.c_str(), &end, 10);
    return !text.empty() && end != nullptr && *end == '\0';
}

[[nodiscard]] inline std::string lower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

} // namespace sortrace::examples
