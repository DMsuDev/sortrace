// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// State and small helpers of the interactive CLI.
#pragma once

#include "common/format.hpp"
#include "common/render.hpp"
#include "common/terminal.hpp"

#include <sortrace/sortrace.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sortrace::examples::cli {

inline constexpr int kMaxSize      = 4096;
inline constexpr int kMaxWatchable = 110; // One terminal column per bar.

struct App
{
    AlgorithmType type       = AlgorithmType::QuickSort;
    int           size       = 32;
    std::uint32_t seed       = 42;
    bool          descending = false;

    bool color   = true;  // ANSI colors and in-place redraw.
    bool ascii   = false; // '#' instead of a block glyph.
    bool logging = false;

    std::chrono::microseconds budget{ 100 }; // Session time slice.
    std::size_t               blockingLimit = SessionOptions{}.blockingLimit;

    std::vector<int>                      data;
    std::shared_ptr<const Recording<int>> recording; // Whatever MODE 1 or MODE 2 produced last.
    std::string                           madeBy;

    void regenerate()
    {
        auto rng = SeedProvider::globalSeed(seed).createEngine(0);
        data = DataSetBuilder::buildShuffled<int>({ .count = size, .maxValue = size }, rng);
        recording.reset(); // It belongs to the previous data.
    }

    [[nodiscard]] Less<int> less() const
    {
        return descending ? Less<int>(std::greater<int>{}) : defaultLess<int>();
    }

    // Library diagnostics go to stderr, only when asked. Without a sink the library is silent.
    [[nodiscard]] LogSink logSink() const
    {
        if (!logging)
            return {};
        return [](LogLevel level, std::string_view message) {
            static constexpr std::array<const char*, 5> names = { "trace", "debug", "info", "warn", "error" };
            std::fprintf(stderr, "  [sortrace %s] %.*s\n", names[static_cast<std::size_t>(level)],
                         static_cast<int>(message.size()), message.data());
        };
    }

    [[nodiscard]] std::string paint(const char* tint, std::string_view text) const
    {
        return examples::paint(color, tint, text);
    }

    [[nodiscard]] RenderOptions renderOptions() const
    {
        return { .title = std::string(toString(type)), .color = color, .ascii = ascii };
    }
};

inline void showCode(const App& app, const std::string& line)
{
    std::printf("  %s\n", app.paint(ansi::dim, "C++  " + line).c_str());
}

inline void printStatus(const App& app, bool indent = true)
{
    std::printf("%s%s%s%s · %d values · seed %u · %s order\n", indent ? "  algorithm " : "",
                app.color ? ansi::bold : "", std::string(toString(app.type)).c_str(), app.color ? ansi::reset : "",
                app.size, app.seed, app.descending ? "descending" : "ascending");
}

inline void printRecording(const App& app)
{
    const Recording<int>& rec = *app.recording;
    std::array<std::size_t, 6> counts{};
    for (const auto& op : rec.operations)
        ++counts[static_cast<std::size_t>(op.type)];

    std::printf("\n  %s  (%s)\n", app.paint(ansi::bold, rec.algorithm).c_str(), app.madeBy.c_str());
    std::printf("    operations   %s   (", thousands(rec.operations.size()).c_str());
    bool first = true;
    for (std::size_t i = 0; i < counts.size(); ++i)
    {
        if (counts[i] == 0)
            continue;
        std::printf("%s%s %s", first ? "" : ", ", std::string(toString(static_cast<OperationType>(i))).c_str(),
                    thousands(counts[i]).c_str());
        first = false;
    }
    std::printf(")\n");
    std::printf("    statistics   comparisons %s · swaps %s · reads %s · writes %s\n",
                thousands(rec.statistics.comparisons).c_str(), thousands(rec.statistics.swaps).c_str(),
                thousands(rec.statistics.reads).c_str(), thousands(rec.statistics.writes).c_str());
    std::printf("    compute      %.2f ms\n", rec.statistics.computeTimeMs);
    std::printf("\n  Recorded. Now try %s or %s.\n\n", app.paint(ansi::bold, "play").c_str(),
                app.paint(ansi::bold, "step").c_str());
}

// rewind > 0 redraws in place (the frame, plus any prompt line printed after it).
inline void drawFrame(const App& app, const Playback<int>& playback, int rewind = 0)
{
    if (rewind > 0 && app.color)
        std::printf("\x1b[%dA\r", rewind);
    std::fputs(renderFrame(app.renderOptions(), playback).c_str(), stdout);
    std::fflush(stdout);
}

} // namespace sortrace::examples::cli
