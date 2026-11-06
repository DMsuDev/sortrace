// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/operation.hpp"
#include "sortrace/core/statistics.hpp"

#include <string>
#include <vector>

namespace sortrace {

/// @brief Contains the complete recorded execution of a sorting algorithm.
///
/// Stores the original input sequence, the operations produced during execution,
/// and the statistics collected by the recorder.
///
/// Recording is a passive data structure with no execution logic. It can be
/// stored, shared, serialized, replayed, or consumed by visualization tools.
///
/// @tparam T The type of values being sorted.
template <SortableValue T>
struct Recording
{
    std::string                algorithm;  ///< Name of the sorting algorithm used.
    std::vector<T>             initial;    ///< Initial state of the data.
    std::vector<Operation<T>>  operations; ///< All operations performed during the run.
    Statistics                 statistics; ///< Collected statistics for the run.
};

} // namespace sortrace
