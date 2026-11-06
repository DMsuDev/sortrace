// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Shared views: one recordShared() run drawn two ways, as bars and as a heatmap.
// Every Playback reads the same operations, so the views always agree.

#include "common/heatmap.hpp"
#include "common/render.hpp"
#include "common/terminal.hpp"

#include <sortrace/sortrace.hpp>

#include <cstdio>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    setupTerminal();
    const bool color = isTerminal();

    constexpr int count = 32;
    auto          rng   = SeedProvider::globalSeed(11).createEngine(0);
    const auto    data  = DataSetBuilder::buildShuffled<int>({ .count = count, .maxValue = count }, rng);

    const auto shared = recordShared<int>(AlgorithmType::ShellSort, data);

    Playback<int> bars;
    Playback<int> heat;
    if (!bars.load(shared) || !heat.load(shared))
        return 1;

    const RenderOptions options{ .title = "Shell Sort", .color = color };
    const std::size_t   total = shared->operations.size();

    for (const std::size_t position : { std::size_t{ 0 }, total / 2, total })
    {
        while (bars.getCurrentOperationIndex() < position)
        {
            if (!bars.stepForward() || !heat.stepForward())
                break;
        }
        std::printf("--- operation %zu of %zu ---\n%s%s\n", bars.getCurrentOperationIndex(), total,
                    renderFrame(options, bars).c_str(), renderHeatmap(heat, color).c_str());
    }
    return 0;
}
