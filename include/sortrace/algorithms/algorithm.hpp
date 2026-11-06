// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/operation.hpp"
#include "sortrace/core/recorder.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace sortrace {

/// Interface of a sorting algorithm that runs one logical operation at a time.
/// Most algorithms should derive from GeneratorSort instead of implementing this directly.
template <SortableValue T>
class SortAlgorithm
{
public:
    virtual ~SortAlgorithm() = default;

    /// Copies @p data as the working buffer and resets the algorithm, so one instance can be reused.
    virtual void load(std::span<const T> data) = 0;

    /// Advances one operation. A run emits any number of operations followed by exactly one Sorted
    /// (also for empty input), then std::nullopt forever. Build operations only through @p recorder.
    virtual std::optional<Operation<T>> step(Recorder<T>& recorder) = 0;

    /// The working copy (sorted once the run completes).
    [[nodiscard]] virtual std::span<const T> getData() const = 0;

    [[nodiscard]] virtual std::string_view getName() const = 0;
    [[nodiscard]] virtual std::string_view getComplexity() const = 0;
    [[nodiscard]] virtual std::string_view getDescription() const = 0;
};

} // namespace sortrace
