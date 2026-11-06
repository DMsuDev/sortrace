// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// sortrace_cli: an interactive tour of sortrace. Only the library and the standard library are used.
//
// It shows the two ways of running an algorithm, and what you can do with the result:
//
//   MODE 1  record()   run to the end in one call. Headless and simple: CLIs, tests, small data.
//   MODE 2  Session    run in time slices, one slice per "frame". For UIs and big data.
//
//   Both produce the same Recording, which Playback replays: by time (play) or by hand (step).
//
// Start it and type `help`. A quick tour:   record   play   session   step   compare
//
// Options: --size N  --algo NAME  --seed N  --plain (no colors, no redraw)  --ascii  --color (force colors)
//
// Layout: app.hpp (state), commands.cpp (record, session, play, step, compare), main.cpp (this file: REPL).

#include "app.hpp"
#include "commands.hpp"

#include "common/input.hpp"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <random>
#include <sstream>

using namespace sortrace;
using namespace sortrace::examples;
using namespace sortrace::examples::cli;

namespace {

void printHelp()
{
    std::printf(
        "\n"
        "  START HERE\n"
        "    tour                 a guided demo, about 2 minutes\n"
        "\n"
        "  RUN an algorithm\n"
        "    record               run it in one call and record every operation\n"
        "    session [blocking]   same result, computed in small time slices (like a UI would)\n"
        "    compare              run every algorithm on the same data and rank them\n"
        "\n"
        "  WATCH the recording\n"
        "    play [speed]         animate it. speed = steps per second, 0 = show the end at once\n"
        "    step                 move through it yourself with the arrow keys (forward and back)\n"
        "    info                 show what the last recording contains\n"
        "\n"
        "  CHANGE the setup\n"
        "    algos                list the algorithms\n"
        "    algo <name>          pick one, e.g. `algo merge`\n"
        "    size <n>             number of values, 4 to %d\n"
        "    new [seed]           shuffle the data again\n"
        "    order asc|desc       sort ascending or descending\n"
        "    data                 print the values\n"
        "    status               show the current setup\n"
        "\n"
        "  ADVANCED\n"
        "    budget <us>          time slice used by `session`, in microseconds\n"
        "    limit <n>            largest input `session blocking` accepts\n"
        "    log on|off           show the library's diagnostics\n"
        "\n"
        "  quit                   leave (Ctrl+D also works)\n\n",
        kMaxSize);
}

void printAlgorithms()
{
    std::printf("\n");
    for (const AlgorithmInfo& info : algorithms())
        std::printf("  %-15s %-12s %.*s\n", std::string(info.meta.name).c_str(), std::string(info.meta.complexity).c_str(),
                    static_cast<int>(info.meta.description.size()), info.meta.description.data());
    std::printf("\n");
}

void printData(const App& app)
{
    std::printf("  ");
    for (std::size_t i = 0; i < app.data.size(); ++i)
        std::printf("%d%s", app.data[i], (i + 1) % 20 == 0 ? "\n  " : " ");
    std::printf("\n");
}

// Returns false to quit.
bool execute(App& app, const std::string& line)
{
    std::istringstream words(line);
    std::string command;
    words >> command;
    command = lower(command);

    std::string rest;
    std::getline(words >> std::ws, rest);

    if (command.empty())
        return true;
    if (command == "quit" || command == "exit" || command == "q")
        return false;

    if (command == "help" || command == "?")
    {
        printHelp();
    }
    else if (command == "algos")
    {
        printAlgorithms();
    }
    else if (command == "status")
    {
        printStatus(app);
    }
    else if (command == "data")
    {
        printData(app);
    }
    else if (command == "record")
    {
        cmdRecord(app);
    }
    else if (command == "compare")
    {
        cmdCompare(app);
    }
    else if (command == "session")
    {
        cmdSession(app, lower(rest) == "blocking");
    }
    else if (command == "tour")
    {
        cmdTour(app);
    }
    else if (command == "step")
    {
        cmdStep(app);
    }
    else if (command == "info")
    {
        if (app.recording)
            printRecording(app);
        else
            std::printf("  Nothing recorded yet: try `record` or `session`.\n");
    }
    else if (command == "play")
    {
        long long perSecond = -1;
        if (!rest.empty() && (!parseInt(rest, perSecond) || perSecond < 0))
            std::printf("  play [steps per second]; 0 plays everything at once.\n");
        else
            cmdPlay(app, static_cast<double>(perSecond));
    }
    else if (command == "algo")
    {
        if (const auto type = parseAlgorithm(rest))
        {
            app.type = *type;
            app.recording.reset();
            printStatus(app);
        }
        else
        {
            std::printf("  Unknown algorithm '%s'. See `algos`.\n", rest.c_str());
        }
    }
    else if (command == "size")
    {
        long long value = 0;
        if (!parseInt(rest, value) || value < 4 || value > kMaxSize)
        {
            std::printf("  size must be between 4 and %d.\n", kMaxSize);
        }
        else
        {
            app.size = static_cast<int>(value);
            app.regenerate();
            printStatus(app);
        }
    }
    else if (command == "new")
    {
        long long value = 0;
        if (rest.empty())
            app.seed = std::random_device{}();
        else if (parseInt(rest, value))
            app.seed = static_cast<std::uint32_t>(value);
        app.regenerate();
        printStatus(app);
    }
    else if (command == "order")
    {
        const std::string which = lower(rest);
        if (which != "asc" && which != "desc")
        {
            std::printf("  order asc|desc\n");
        }
        else
        {
            app.descending = which == "desc";
            app.recording.reset();
            printStatus(app);
        }
    }
    else if (command == "budget")
    {
        long long value = 0;
        if (!parseInt(rest, value) || value < 1)
            std::printf("  budget <microseconds>, at least 1.\n");
        else
            app.budget = std::chrono::microseconds(value);
    }
    else if (command == "limit")
    {
        long long value = 0;
        if (!parseInt(rest, value) || value < 1)
            std::printf("  limit <elements>, at least 1.\n");
        else
            app.blockingLimit = static_cast<std::size_t>(value);
    }
    else if (command == "log")
    {
        app.logging = lower(rest) == "on";
        std::printf("  library logging %s\n", app.logging ? "on (stderr)" : "off");
    }
    else
    {
        std::printf("  Unknown command '%s'. Type `help`.\n", command.c_str());
    }

    return true;
}

} // namespace

int main(int argc, char** argv)
{
    App app;
    app.color = isTerminal();
    bool forceColor = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        const auto value = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        long long number = 0;

        if (arg == "--plain")
        {
            app.color = false;
        }
        else if (arg == "--color")
        {
            app.color  = true;
            forceColor = true;
        }
        else if (arg == "--ascii")
        {
            app.ascii = true;
        }
        else if (arg == "--size")
        {
            if (parseInt(value(), number) && number >= 4 && number <= kMaxSize)
                app.size = static_cast<int>(number);
        }
        else if (arg == "--seed")
        {
            if (parseInt(value(), number))
                app.seed = static_cast<std::uint32_t>(number);
        }
        else if (arg == "--algo")
        {
            if (const auto type = parseAlgorithm(value()))
                app.type = *type;
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::printf("sortrace_cli [--size N] [--algo NAME] [--seed N] [--plain] [--ascii] [--color]\n");
            return 0;
        }
    }

    if (app.color && !setupTerminal() && !forceColor)
        app.color = false;
    app.regenerate();

    std::printf("\n  %s  ·  see how sorting algorithms work, step by step\n", app.paint(ansi::bold, "sortrace").c_str());
    std::printf("  %s\n\n", app.paint(ansi::dim, "An algorithm runs once and everything it does is recorded. Then you replay it.").c_str());
    std::printf("  Quick start\n");
    std::printf("    %s     a guided demo, about 2 minutes (best place to begin)\n", app.paint(ansi::bold, "tour").c_str());
    std::printf("    %s   run the algorithm and record it\n", app.paint(ansi::bold, "record").c_str());
    std::printf("    %s     watch it as an animation\n", app.paint(ansi::bold, "play").c_str());
    std::printf("    %s     move through it yourself, forward and backward\n", app.paint(ansi::bold, "step").c_str());
    std::printf("    %s  compare every algorithm on the same data\n", app.paint(ansi::bold, "compare").c_str());
    std::printf("    %s     all commands\n\n", app.paint(ansi::bold, "help").c_str());
    std::printf("  Now: ");
    printStatus(app, false);
    std::printf("\n");

    std::string line;
    for (;;)
    {
        std::printf("sortrace> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line))
            break;

        try
        {
            if (!execute(app, line))
                break;
        }
        catch (const std::exception& e)
        {
            std::printf("  error: %s\n", e.what());
        }
    }
    return 0;
}
