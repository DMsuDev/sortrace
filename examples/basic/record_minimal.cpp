// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Record: the simplest use. Run an algorithm to the end in one call and read its statistics.

#include "common/format.hpp"

#include <sortrace/sortrace.hpp>

#include <cstdio>
#include <vector>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    const std::vector<int> data = { 5, 2, 9, 1, 7, 3, 8, 4, 6 };

    const auto recording = record<int>(AlgorithmType::QuickSort, data);

    std::printf("%s on %zu values\n", recording.algorithm.c_str(), data.size());
    std::printf("  operations  %s\n", thousands(recording.operations.size()).c_str());
    std::printf("  comparisons %s\n", thousands(recording.statistics.comparisons).c_str());
    std::printf("  swaps       %s\n", thousands(recording.statistics.swaps).c_str());
    std::printf("  compute     %.3f ms\n", recording.statistics.computeTimeMs);
    return 0;
}
