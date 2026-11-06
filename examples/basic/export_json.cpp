// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Export JSON: a Recording is plain data, so it can leave the program as JSON for any other tool.

#include <sortrace/sortrace.hpp>

#include <cstdio>
#include <string>
#include <vector>

using namespace sortrace;

// Enough for names and numbers: quotes, backslashes and control characters.
static std::string jsonEscape(const std::string& text)
{
    std::string out;
    for (const char c : text)
    {
        if (c == '"' || c == '\\')
        {
            out += '\\';
            out += c;
        }
        else if (static_cast<unsigned char>(c) < 0x20)
        {
            char code[8];
            std::snprintf(code, sizeof code, "\\u%04x", static_cast<unsigned>(c));
            out += code;
        }
        else
        {
            out += c;
        }
    }
    return out;
}

int main()
{
    const std::vector<int> data = { 5, 2, 9, 1, 7, 3 };
    const auto             rec  = record<int>(AlgorithmType::BubbleSort, data);

    std::printf("{\n  \"algorithm\": \"%s\",\n  \"comparisons\": %llu,\n  \"swaps\": %llu,\n  \"operations\": [\n",
                jsonEscape(rec.algorithm).c_str(), static_cast<unsigned long long>(rec.statistics.comparisons),
                static_cast<unsigned long long>(rec.statistics.swaps));
    for (std::size_t i = 0; i < rec.operations.size(); ++i)
    {
        const auto& op = rec.operations[i];
        std::printf("    {\"type\": \"%s\", \"a\": %d, \"b\": %d}%s\n", std::string(toString(op.type)).c_str(), op.indexA,
                    op.indexB, i + 1 < rec.operations.size() ? "," : "");
    }
    std::printf("  ]\n}\n");
    return 0;
}
