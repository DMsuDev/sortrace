// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Implementation of the CLI commands: record, compare, session, play, step and tour.

#include "commands.hpp"

#include "common/input.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <thread>

namespace sortrace::examples::cli {

using Clock = std::chrono::steady_clock;

// ---------------------------------------------------------------------------------------------------
//  MODE 1: record()
// ---------------------------------------------------------------------------------------------------

void cmdRecord(App& app)
{
    showCode(app, "auto recording = record<int>(AlgorithmType::" + enumName(app.type) + ", data" +
                      (app.descending ? ", std::greater<int>{}" : "") + ");");
    try
    {
        auto recording = record<int>(app.type, app.data, app.less());
        app.recording  = std::make_shared<const Recording<int>>(std::move(recording));
        app.madeBy     = "record()";
        printRecording(app);
    }
    catch (const std::exception& e) // Only a buggy algorithm gets here; expected failures are values.
    {
        std::printf("  The algorithm threw: %s\n", e.what());
    }
}

void cmdCompare(const App& app)
{
    showCode(app, "for (AlgorithmType type : getAvailableAlgorithms()) record<int>(type, data);");
    if (app.size > 1500)
        std::printf("  %d values: the O(n²) algorithms take a moment...\n", app.size);

    struct Row { std::string name; Statistics stats; };
    std::vector<Row> rows;
    for (const AlgorithmType type : getAvailableAlgorithms())
    {
        auto recording = record<int>(type, app.data, app.less());
        rows.push_back({ recording.algorithm, recording.statistics });
    }

    const auto best = [&](auto member) {
        std::uint64_t lowest = ~std::uint64_t{ 0 };
        for (const Row& row : rows)
            lowest = std::min<std::uint64_t>(lowest, row.stats.*member);
        return lowest;
    };
    const std::uint64_t bestCompares = best(&Statistics::comparisons);
    const std::uint64_t bestSwaps    = best(&Statistics::swaps);
    const std::uint64_t bestOps      = best(&Statistics::operationCount);

    const auto cellText = [&](std::uint64_t value, std::uint64_t lowest) {
        char text[40];
        std::snprintf(text, sizeof text, "%11s%s", thousands(value).c_str(), value == lowest ? "*" : " ");
        return value == lowest ? app.paint(ansi::green, text) : std::string(text);
    };

    std::printf("\n  Every algorithm on the same %d values (seed %u). * = fewest.\n\n", app.size, app.seed);
    std::printf("  %-15s %12s  %12s  %12s  %10s\n", "Algorithm", "Comparisons", "Swaps", "Operations", "Time (ms)");
    for (const Row& row : rows)
        std::printf("  %-15s %s %s %s %10.2f\n", row.name.c_str(), cellText(row.stats.comparisons, bestCompares).c_str(),
                    cellText(row.stats.swaps, bestSwaps).c_str(), cellText(row.stats.operationCount, bestOps).c_str(),
                    row.stats.computeTimeMs);
    std::printf("\n");
}

// ---------------------------------------------------------------------------------------------------
//  MODE 2: Session
// ---------------------------------------------------------------------------------------------------

namespace {

std::string explain(ComputeError error, const App& app)
{
    switch (error)
    {
        case ComputeError::NoAlgorithm:    return "no algorithm was set";
        case ComputeError::NoData:         return "there is no data";
        case ComputeError::AlgorithmThrew: return "the algorithm failed; partial results were discarded";
        case ComputeError::TooLargeForBlocking:
            return "too many values (" + thousands(static_cast<std::uint64_t>(app.size)) + ") to compute in one go; the limit is " +
                   thousands(app.blockingLimit);
    }
    return "";
}

// A bar that fills as pieces are computed. The total is unknown until the end, so it only shows activity.
std::string activityBar(int pieces)
{
    constexpr int width = 24;
    const int     filled = std::min(pieces, width);
    std::string   bar = "[";
    for (int i = 0; i < width; ++i)
        bar += i < filled ? "#" : ".";
    return bar + "]";
}

} // namespace

void cmdSession(App& app, bool blocking)
{
    Session<int> session(SessionOptions{ .blockingLimit = app.blockingLimit });
    session.setLogSink(app.logSink());
    session.setAlgorithm(make<int>(app.type, app.less()));
    session.setData(app.data);

    if (blocking)
    {
        std::printf("  Computing everything in one go. Fine for small inputs; a UI would freeze on big ones.\n");
        showCode(app, "auto result = session.compute();   // no time limit: run to the end, or an error");
        const auto result = session.compute(); // std::expected<Progress, ComputeError>
        if (!result)
        {
            std::printf("  %s: %s\n", app.paint(ansi::red, "Refused").c_str(), explain(result.error(), app).c_str());
            std::printf("  Use `session` instead (computes in small pieces), or raise the limit with `limit %d`.\n",
                        std::max(app.size, 1));
            return;
        }
    }
    else
    {
        std::printf("  Big inputs would freeze a UI if computed in one go. A Session computes a little at a time\n"
                    "  and gives control back between pieces, so the UI can keep drawing.\n");
        showCode(app, "while (session.compute(budget) == Progress::InProgress) { /* draw a frame */ }");
        std::printf("  time per piece: %lld µs\n\n", static_cast<long long>(app.budget.count()));

        int pieces = 0;
        for (;;)
        {
            const auto result = session.compute(app.budget);
            ++pieces;
            if (!result)
            {
                std::printf("\n  %s: %s\n", app.paint(ansi::red, "Failed").c_str(), explain(result.error(), app).c_str());
                return;
            }

            // Partial statistics are readable while the run is still going.
            std::printf("\r  Computing  %s  piece %d  ·  %s operations so far%s", activityBar(pieces).c_str(), pieces,
                        thousands(session.getStatistics().operationCount).c_str(), app.color ? "\x1b[K" : "");
            std::fflush(stdout);

            if (*result == Progress::Complete)
                break;
            if (app.color && pieces < 300) // Pretend to be a UI waiting for the next frame.
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        std::printf("\n\n  Done in %d piece%s of at most %lld µs each. A UI could draw a frame between them.\n", pieces,
                    pieces == 1 ? "" : "s", static_cast<long long>(app.budget.count()));
        if (pieces == 1)
            std::printf("  It all fit in a single piece. Try `algo bubble` and `size 400`, or `budget 1`.\n");
    }

    app.recording = session.getRecording(); // Shared and immutable.
    app.madeBy    = blocking ? "session, in one go" : "session, in pieces";
    std::printf("  The result is identical to `record`: only how it was computed differs.\n");
    printRecording(app);
}

// ---------------------------------------------------------------------------------------------------
//  Playback: by time (play) or by hand (step)
// ---------------------------------------------------------------------------------------------------

namespace {

bool prepare(App& app, Playback<int>& playback)
{
    if (app.size > kMaxWatchable)
    {
        std::printf("  To watch it, use at most %d values (the engine itself handles %d). Try: size 40\n", kMaxWatchable, kMaxSize);
        return false;
    }
    if (!app.recording)
    {
        std::printf("  No recording yet: running record() first.\n");
        cmdRecord(app);
        if (!app.recording)
            return false;
    }

    playback.setLogSink(app.logSink());
    if (const auto loaded = playback.load(app.recording); !loaded) // The recording is shared, not copied.
    {
        std::printf("  playback.load() refused the recording.\n");
        return false;
    }
    return true;
}

} // namespace

void cmdPlay(App& app, double stepsPerSecond)
{
    Playback<int> playback;
    if (!prepare(app, playback))
        return;

    if (stepsPerSecond < 0) // Auto: about eight seconds, whatever the algorithm.
        stepsPerSecond = std::max(30.0, static_cast<double>(playback.getTotalOperations()) / 8.0);
    const bool instant = stepsPerSecond == 0.0 || !app.color; // Without ANSI there is no animation to show.

    showCode(app, instant ? std::string("playback.enableDelay(false); playback.play(); playback.update(0);   // everything at once")
                          : "playback.setDelayMs(" + std::to_string(static_cast<int>(1000.0 / stepsPerSecond)) +
                                "); playback.play(); /* each frame: */ playback.update(dt);");

    playback.enableDelay(!instant);
    if (!instant)
        playback.setDelayMs(static_cast<float>(1000.0 / stepsPerSecond));
    playback.play();

    if (instant)
    {
        playback.update(0.0f);
        drawFrame(app, playback);
        return;
    }

    drawFrame(app, playback);
    auto        last  = Clock::now();
    std::size_t shown = playback.getCurrentOperationIndex();
    while (playback.isPlaying())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 frames per second.
        const auto now = Clock::now();
        playback.update(std::chrono::duration<float>(now - last).count());
        last = now;

        if (playback.getCurrentOperationIndex() != shown || !playback.isPlaying())
        {
            shown = playback.getCurrentOperationIndex();
            drawFrame(app, playback, kFrameLines);
        }
    }
}

// ---------------------------------------------------------------------------------------------------
//  step: two ways to drive it. Keys in a real terminal; full-word commands when input is piped.
// ---------------------------------------------------------------------------------------------------

namespace {

constexpr long long kJump = 10;

void jump(Playback<int>& playback, long long count, bool forward)
{
    for (long long i = 0; i < count && (forward ? playback.stepForward() : playback.stepBackward()); ++i) {}
}

void stepWithKeys(App& app, Playback<int>& playback)
{
    const auto hint = [&] {
        const auto key = [&](const char* name) { return app.paint(ansi::bold, name); };
        std::printf("  %s back   %s next   %s/%s %lld steps   %s start   %s end   %s quit%s\n", key("←").c_str(),
                    key("→").c_str(), key("↑").c_str(), key("↓").c_str(), kJump, key("Home").c_str(),
                    key("End").c_str(), key("q").c_str(), "\x1b[K");
        std::fflush(stdout);
    };

    const RawInput raw;
    drawFrame(app, playback);
    hint();

    for (;;)
    {
        switch (readKey())
        {
            case Key::Next:        jump(playback, 1, true);       break;
            case Key::Back:        jump(playback, 1, false);      break;
            case Key::JumpForward: jump(playback, kJump, true);   break;
            case Key::JumpBack:    jump(playback, kJump, false);  break;
            case Key::End:         while (playback.stepForward()) {} break;
            case Key::Start:       playback.stop();               break;
            case Key::Quit:
            case Key::Closed:      return;
            case Key::Other:       continue;
        }
        drawFrame(app, playback, kFrameLines + 1); // The frame and the hint line under it.
        hint();
    }
}

void stepWithCommands(App& app, Playback<int>& playback)
{
    std::printf("  Commands: next [n] · back [n] · start · end · quit      (Enter alone = next)\n");
    drawFrame(app, playback);

    std::string line;
    for (;;)
    {
        std::printf("step> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line))
            return;

        std::istringstream words(line);
        std::string command;
        long long count = 1;
        words >> command;
        if (std::string amount; words >> amount && (!parseInt(amount, count) || count < 1))
            count = 1;
        command = lower(command);

        if (command == "quit" || command == "q" || command == "exit")
            return;
        else if (command.empty() || command == "next" || command == "n")
            jump(playback, count, true);
        else if (command == "back" || command == "b")
            jump(playback, count, false);
        else if (command == "end" || command == "e")
            while (playback.stepForward()) {}
        else if (command == "start" || command == "restart" || command == "r")
            playback.stop();
        else
        {
            std::printf("  Unknown command '%s'. Use: next, back, start, end or quit.\n", command.c_str());
            continue;
        }
        drawFrame(app, playback, kFrameLines + 1); // The frame and the prompt line under it.
    }
}

} // namespace

void cmdStep(App& app)
{
    Playback<int> playback;
    if (!prepare(app, playback))
        return;

    showCode(app, "playback.stepForward();   playback.stepBackward();   // no clock involved");
    if (app.color && isInteractiveInput())
        stepWithKeys(app, playback);
    else
        stepWithCommands(app, playback);
}

// ---------------------------------------------------------------------------------------------------
//  tour: a guided demo that runs the other commands for you
// ---------------------------------------------------------------------------------------------------

namespace {

void tourTitle(const App& app, int number, const char* title)
{
    std::printf("\n  %s\n", app.paint(ansi::bold, ("Step " + std::to_string(number) + " of 4  ·  " + title).c_str()).c_str());
}

// Waits for Enter only when a person is typing; piped runs and --plain go straight through.
void tourPause(const App& app)
{
    if (!app.color || !isInteractiveInput())
        return;
    std::printf("\n  %s", app.paint(ansi::dim, "Press Enter to continue... ").c_str());
    std::fflush(stdout);
    std::string ignored;
    std::getline(std::cin, ignored);
}

} // namespace

void cmdTour(const App& current)
{
    App demo = current; // Work on a copy: the user's own setup stays untouched.
    demo.type       = AlgorithmType::QuickSort;
    demo.size       = 12;
    demo.descending = false;
    demo.regenerate();

    std::printf("\n  %s\n", demo.paint(ansi::bold, "Guided tour (about 2 minutes)").c_str());
    std::printf("  Quick Sort on 12 values. Your own settings are not changed.\n");
    tourPause(demo);

    tourTitle(demo, 1, "Record");
    std::printf("  The algorithm runs once, and every comparison and swap it makes is written down.\n\n");
    cmdRecord(demo);
    if (!demo.recording)
        return;
    tourPause(demo);

    tourTitle(demo, 2, "Watch");
    std::printf("  That recording can be played like a video.\n\n");
    cmdPlay(demo, 8.0);
    tourPause(demo);

    tourTitle(demo, 3, "Go backward");
    std::printf("  Everything was recorded, so you can also move backward, which an algorithm can't do on its own.\n\n");
    {
        Playback<int> playback;
        if (!prepare(demo, playback))
            return;
        showCode(demo, "playback.stepForward();   playback.stepBackward();");
        constexpr int steps = 20;
        for (int i = 0; i < steps; ++i)
            playback.stepForward();
        std::printf("  After %d steps forward:\n", steps);
        drawFrame(demo, playback);
        for (int i = 0; i < steps; ++i)
            playback.stepBackward();
        std::printf("\n  After %d steps back, we are at the start again:\n", steps);
        drawFrame(demo, playback);
    }
    tourPause(demo);

    tourTitle(demo, 4, "Compare");
    std::printf("  The same data through every algorithm. The least work in each column is marked with *.\n");
    cmdCompare(demo);

    std::printf("  %s\n", demo.paint(ansi::bold, "That's the tour. Now it's your turn:").c_str());
    std::printf("    algo merge     pick another algorithm        size 40   use more values\n");
    std::printf("    step           move through it with the arrow keys\n");
    std::printf("    help           every command\n\n");
}

} // namespace sortrace::examples::cli

