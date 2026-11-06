// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Text formatting of library values, shared by the examples.
#pragma once

#include <sortrace/sortrace.hpp>

#include <cstdint>
#include <cstdio>
#include <string>

namespace sortrace::examples {

[[nodiscard]] inline std::string thousands(std::uint64_t value)
{
    std::string digits = std::to_string(value);
    for (std::size_t i = digits.size(); i > 3; i -= 3)
        digits.insert(i - 3, ",");
    return digits;
}

// "Quick Sort" -> "QuickSort": the enumerator name, for the code shown to the user.
[[nodiscard]] inline std::string enumName(AlgorithmType type)
{
    std::string name(toString(type));
    std::erase(name, ' ');
    return name;
}

[[nodiscard]] inline std::string markName(MarkId id)
{
    std::string name(toString(id.role));
    return id.role == MarkRole::Custom ? name + "#" + std::to_string(id.custom) : name;
}

// What an operation does, in one line.
[[nodiscard]] inline std::string describe(const Operation<int>& op)
{
    char text[128];
    switch (op.type)
    {
        case OperationType::Compare:
            std::snprintf(text, sizeof text, "Compare    [%d]=%d  vs  [%d]=%d", op.indexA, op.valueA, op.indexB, op.valueB);
            break;
        case OperationType::Swap:
            std::snprintf(text, sizeof text, "Swap       [%d] <-> [%d]   (%d and %d)", op.indexA, op.indexB, op.valueA, op.valueB);
            break;
        case OperationType::Overwrite:
            std::snprintf(text, sizeof text, "Overwrite  [%d] = %d   (was %d)", op.indexA, op.valueA, op.valueB);
            break;
        case OperationType::MarkSet:
            std::snprintf(text, sizeof text, "MarkSet    %s -> [%d]", markName(op.mark).c_str(), op.indexA);
            break;
        case OperationType::MarkClear:
            std::snprintf(text, sizeof text, "MarkClear  %s", markName(op.mark).c_str());
            break;
        case OperationType::Sorted:
            std::snprintf(text, sizeof text, "Sorted     the run is over");
            break;
    }
    return text;
}

} // namespace sortrace::examples
