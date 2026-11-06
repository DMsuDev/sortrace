// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Drawing a Playback as bars in the terminal. This is the part a real UI replaces: the library only says
// WHAT is where, and this file decides how it looks.
#pragma once

#include "format.hpp"
#include "terminal.hpp"

#include <sortrace/sortrace.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sortrace::examples {

inline constexpr int kBarRows    = 12;
inline constexpr int kFrameLines = kBarRows + 4; // title + bars + next + marks + legend

struct RenderOptions
{
    std::string title;
    bool        color = true;  // ANSI colors and in-place redraw.
    bool        ascii = false; // '#' instead of a block glyph.
};

[[nodiscard]] inline std::string paint(bool color, const char* tint, std::string_view text)
{
    if (!color)
        return std::string(text);
    return std::string(tint) + std::string(text) + ansi::reset;
}

namespace detail {

enum class Cell { Idle, Settled, Active, Marked };

inline const char* markTint(MarkRole role)
{
    switch (role)
    {
        case MarkRole::Pivot:   return ansi::magenta;
        case MarkRole::Minimum: return ansi::cyan;
        default:                return ansi::blue;
    }
}

inline const char* stateName(Playback<int>::State state)
{
    switch (state)
    {
        case Playback<int>::State::Stopped:  return "STOPPED";
        case Playback<int>::State::Playing:  return "PLAYING";
        case Playback<int>::State::Paused:   return "PAUSED";
        case Playback<int>::State::Finished: return "FINISHED";
    }
    return "";
}

} // namespace detail

// Exactly kFrameLines lines, so it can be redrawn in place.
[[nodiscard]] inline std::string renderFrame(const RenderOptions& opt, const Playback<int>& playback)
{
    using detail::Cell;

    const auto  data = playback.getData();
    const int   top  = *std::ranges::max_element(data);
    const bool  gap  = data.size() <= 56;
    const char* eol  = opt.color ? "\x1b[K" : "";

    // 1. What is each bar right now? Priority: mark > about-to-change > settled > idle.
    std::vector<Cell>        cell(data.size(), Cell::Idle);
    std::vector<const char*> tint(data.size(), ansi::dim);

    for (std::size_t slot = 0; slot < data.size(); ++slot)
    {
        if (playback.isSettled(slot)) // Final value already in place (exact in both directions).
        {
            cell[slot] = Cell::Settled;
            tint[slot] = ansi::green;
        }
    }

    if (const Operation<int>* next = playback.getCurrentOperation())
    {
        const char* active = nullptr;
        switch (next->type)
        {
            case OperationType::Compare:   active = ansi::yellow;  break;
            case OperationType::Swap:
            case OperationType::Overwrite: active = ansi::red;     break;
            case OperationType::MarkSet:   active = ansi::magenta; break;
            default: break;
        }
        for (const int index : { next->indexA, next->indexB })
        {
            if (active && index >= 0)
            {
                cell[static_cast<std::size_t>(index)] = Cell::Active;
                tint[static_cast<std::size_t>(index)] = active;
            }
        }
    }

    for (const auto& [id, index] : playback.getMarks()) // Marks follow their element through swaps.
    {
        cell[static_cast<std::size_t>(index)] = Cell::Marked;
        tint[static_cast<std::size_t>(index)] = detail::markTint(id.role);
    }

    const auto glyph = [&](Cell c) -> std::string {
        if (opt.color)
            return opt.ascii ? "#" : "\xE2\x96\x88"; // full block
        switch (c) // Without colors the state is told by the character.
        {
            case Cell::Settled: return "=";
            case Cell::Active:  return "*";
            case Cell::Marked:  return "@";
            case Cell::Idle:    break;
        }
        return "#";
    };

    // 2. Compose the frame.
    std::string out;
    char line[256];

    std::snprintf(line, sizeof line, "  %s  %s   step %zu / %zu   %.0f%%", paint(opt.color, ansi::bold, opt.title).c_str(),
                  detail::stateName(playback.getState()), playback.getCurrentOperationIndex(),
                  playback.getTotalOperations(), static_cast<double>(playback.getProgress()) * 100.0);
    out += line; out += eol; out += '\n';

    for (int row = kBarRows; row >= 1; --row)
    {
        out += "  ";
        for (std::size_t i = 0; i < data.size(); ++i)
        {
            const int height = std::max(1, (data[i] * kBarRows + top - 1) / top);
            if (height >= row)
            {
                if (opt.color) out += tint[i];
                out += glyph(cell[i]);
                if (opt.color) out += ansi::reset;
            }
            else
            {
                out += ' ';
            }
            if (gap) out += ' ';
        }
        out += eol; out += '\n';
    }

    const Operation<int>* next = playback.getCurrentOperation();
    out += "  next   " + (next ? describe(*next) : std::string("-")) + eol + "\n";

    out += "  marks  ";
    if (playback.getMarks().empty())
        out += "-";
    for (const auto& [id, index] : playback.getMarks())
        out += paint(opt.color, detail::markTint(id.role), markName(id) + "@[" + std::to_string(index) + "]") + "  ";
    out += eol; out += '\n';

    if (opt.color)
        out += "  legend " + paint(true, ansi::dim, "idle") + "  " + paint(true, ansi::yellow, "compare") + "  " +
               paint(true, ansi::red, "swap/write") + "  " + paint(true, ansi::magenta, "mark") + "  " +
               paint(true, ansi::green, "settled") + eol + "\n";
    else
        out += std::string("  legend # idle  * about to change  @ mark  = settled") + eol + "\n";

    return out;
}

} // namespace sortrace::examples
