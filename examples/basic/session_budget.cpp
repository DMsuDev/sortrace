// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Session: compute in small pieces, so a UI never waits for the whole run.
// Each turn of the loop stands for one frame; a real UI would draw something in it.

#include "common/format.hpp"

#include <sortrace/sortrace.hpp>

#include <chrono>
#include <cstdio>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    constexpr int count = 1000;
    auto          rng   = SeedProvider::globalSeed(42).createEngine(0);
    const auto    data  = DataSetBuilder::buildShuffled<int>({ .count = count, .maxValue = count }, rng);

    Session<int> session;
    session.setAlgorithm(make<int>(AlgorithmType::BubbleSort, defaultLess<int>()));
    session.setData(data);

    constexpr std::chrono::microseconds budget{ 200 };
    int                                 slices = 0;
    for (;;)
    {
        const auto result = session.compute(budget);
        if (!result)
        {
            std::printf("compute() failed\n");
            return 1;
        }
        ++slices;
        std::printf("slice %3d: %s operations so far\n", slices, thousands(session.getStatistics().operationCount).c_str());
        if (*result == Progress::Complete)
            break;
        // A real UI would draw a frame here.
    }

    const auto recording = session.getRecording();
    std::printf("done in %d slices, %s operations\n", slices, thousands(recording->operations.size()).c_str());
    return 0;
}
