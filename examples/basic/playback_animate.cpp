// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Playback by time: call update(dt) once per frame and redraw when the position changes.

#include "common/render.hpp"
#include "common/terminal.hpp"

#include <sortrace/sortrace.hpp>

#include <chrono>
#include <cstdio>
#include <thread>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    setupTerminal();

    constexpr int count = 40;
    auto          rng   = SeedProvider::globalSeed(7).createEngine(0);
    const auto    data  = DataSetBuilder::buildShuffled<int>({ .count = count, .maxValue = count }, rng);

    Playback<int> playback;
    if (!playback.load(recordShared<int>(AlgorithmType::QuickSort, data)))
        return 1;
    playback.setDelayMs(30.0f);
    playback.play();

    const RenderOptions options{ .title = "Quick Sort", .color = isTerminal() };
    std::fputs(renderFrame(options, playback).c_str(), stdout);

    auto        last  = std::chrono::steady_clock::now();
    std::size_t shown = playback.getCurrentOperationIndex();
    while (playback.isPlaying())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        const auto now = std::chrono::steady_clock::now();
        playback.update(std::chrono::duration<float>(now - last).count());
        last = now;

        if (playback.getCurrentOperationIndex() == shown && playback.isPlaying())
            continue;
        shown = playback.getCurrentOperationIndex();
        if (options.color)
            std::printf("\x1b[%dA\r", kFrameLines);
        std::fputs(renderFrame(options, playback).c_str(), stdout);
        std::fflush(stdout);
    }
    return 0;
}
