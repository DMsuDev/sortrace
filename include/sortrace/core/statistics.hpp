// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace sortrace {

/// Counters of one run. Every Session owns its own; never share them.
struct Statistics
{
    std::uint64_t comparisons    = 0;
    std::uint64_t swaps          = 0;
    std::uint64_t reads          = 0;
    std::uint64_t writes         = 0;
    std::uint64_t operationCount = 0;

    /// Time spent running the algorithm (never playback time).
    double computeTimeMs = 0.0;
};

} // namespace sortrace
