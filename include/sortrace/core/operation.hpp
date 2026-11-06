// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/concepts.hpp"
#include "sortrace/core/mark.hpp"

#include <cstdint>
#include <string_view>

namespace sortrace {

enum class OperationType : std::uint8_t
{
    Compare,   // Two elements were compared. Data unchanged.
    Swap,      // Two elements exchanged places.
    Overwrite, // One slot received a new value.
    MarkSet,   // A mark now points at an element (it follows that element through swaps).
    MarkClear, // A mark was removed.
    Sorted     // The run is over. Always the last operation.
};

/// One recorded event. The whole list is enough to replay a run forward and backward.
///
/// | type      | indexA        | indexB | valueA            | valueB         |
/// |-----------|---------------|--------|-------------------|----------------|
/// | Compare   | first slot    | second | value at indexA   | value at indexB|
/// | Swap      | first slot    | second | value at indexA*  | value at indexB*|
/// | Overwrite | slot written  | -      | new value         | replaced value |
/// | MarkSet   | marked slot   | -      | marked value      | -              |
/// | MarkClear | -             | -      | -                 | -              |
/// | Sorted    | -             | -      | -                 | -              |
///
/// (*) captured before the swap. Unused indices are -1, `mark` only matters for Mark* types.
template <SortableValue T>
struct Operation
{
    OperationType type   = OperationType::Compare;
    int           indexA = -1;
    int           indexB = -1;
    T             valueA{};
    T             valueB{};
    MarkId        mark{};
};

[[nodiscard]] constexpr std::string_view toString(OperationType type) noexcept
{
    switch (type)
    {
        case OperationType::Compare:   return "Compare";
        case OperationType::Swap:      return "Swap";
        case OperationType::Overwrite: return "Overwrite";
        case OperationType::MarkSet:   return "MarkSet";
        case OperationType::MarkClear: return "MarkClear";
        case OperationType::Sorted:    return "Sorted";
    }
    return "Unknown";
}

} // namespace sortrace
