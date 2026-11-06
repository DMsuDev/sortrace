// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Step backward: replay by hand, forward and backward, with no clock.
// Every operation is reversible, so going back returns exactly the original data.

#include "common/render.hpp"

#include <sortrace/sortrace.hpp>

#include <algorithm>
#include <cstdio>
#include <vector>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    constexpr int count = 20;
    auto          rng   = SeedProvider::globalSeed(3).createEngine(0);
    const auto    data  = DataSetBuilder::buildShuffled<int>({ .count = count, .maxValue = count }, rng);

    Playback<int> playback;
    if (!playback.load(recordShared<int>(AlgorithmType::InsertionSort, data)))
        return 1;

    const std::vector<int> before(playback.getData().begin(), playback.getData().end());

    constexpr int steps = 25;
    for (int i = 0; i < steps; ++i)
        playback.stepForward();

    const RenderOptions options{ .title = "Insertion Sort", .color = false };
    std::printf("After %d steps forward:\n%s\n", steps, renderFrame(options, playback).c_str());

    for (int i = 0; i < steps; ++i)
        playback.stepBackward();

    const bool same = std::ranges::equal(before, playback.getData());
    std::printf("After %d steps back, the data is the original again: %s\n", steps, same ? "yes" : "NO");
    return same ? 0 : 1;
}
