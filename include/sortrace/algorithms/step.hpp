// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/concepts.hpp"
#include "sortrace/core/mark.hpp"

#include <cstddef>

namespace sortrace {

/// One thing an algorithm wants to do next. Describe it with `co_yield Step<T>::...`;
/// GeneratorSort performs it through the Recorder. Build steps with the factories, never by hand.
template <SortableValue T>
struct Step
{
    enum class Kind { Compare, Swap, Overwrite, Mark, ClearMark };

    Kind        kind = Kind::Compare;
    std::size_t a    = 0;
    std::size_t b    = 0;
    T           value{};
    bool        aux  = false; // Compare: read from the auxiliary buffer.
    MarkId      markId{};

    /// Compare data[i] with data[j]. No change.
    static Step compare(std::size_t i, std::size_t j)    { return make(Kind::Compare, i, j); }
    /// Compare aux[i] with aux[j] (merge-style algorithms). No change.
    static Step compareAux(std::size_t i, std::size_t j) { Step s = make(Kind::Compare, i, j); s.aux = true; return s; }
    /// Swap data[i] and data[j].
    static Step swap(std::size_t i, std::size_t j)       { return make(Kind::Swap, i, j); }
    /// Write @p newValue into data[i].
    static Step overwrite(std::size_t i, T newValue)     { Step s = make(Kind::Overwrite, i, 0); s.value = std::move(newValue); return s; }
    /// Point mark @p id at data[i]. The mark follows that element through swaps.
    static Step mark(MarkId id, std::size_t i)           { Step s = make(Kind::Mark, i, 0); s.markId = id; return s; }
    /// Remove mark @p id.
    static Step clearMark(MarkId id)                     { Step s = make(Kind::ClearMark, 0, 0); s.markId = id; return s; }

private:
    static Step make(Kind kind, std::size_t a, std::size_t b)
    {
        Step s;
        s.kind = kind;
        s.a    = a;
        s.b    = b;
        return s;
    }
};

} // namespace sortrace
