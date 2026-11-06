// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// A second way to draw the same Playback: one row, one cell per value, brighter = bigger.
// Like render.hpp, this is only one possible look; the library knows nothing about it.
#pragma once

#include "terminal.hpp"

#include <sortrace/sortrace.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace sortrace::examples {

// Cold to hot, as 256-color codes.
inline constexpr std::array<int, 8> kHeatPalette = { 17, 21, 33, 45, 48, 118, 214, 196 };

[[nodiscard]] inline std::string renderHeatmap(const Playback<int>& playback, bool color)
{
    constexpr std::string_view ramp = " .:-=+*#%@";

    const auto data = playback.getData();
    const int  top  = *std::ranges::max_element(data);

    std::string out = "  ";
    for (const int value : data)
    {
        const std::size_t level = static_cast<std::size_t>((value - 1) * 7 / std::max(top - 1, 1));
        if (color)
            out += "\x1b[48;5;" + std::to_string(kHeatPalette[level]) + "m  " + ansi::reset;
        else
            out += std::string(2, ramp[level * (ramp.size() - 1) / 7]);
    }
    return out + "\n";
}

} // namespace sortrace::examples
