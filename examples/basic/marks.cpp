// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Marks: named positions (pivot, minimum...) that follow their element through swaps.

#include "common/format.hpp"

#include <sortrace/sortrace.hpp>

#include <cstdio>
#include <vector>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    const std::vector<int> data = { 6, 3, 8, 1, 9, 2, 7, 4, 5 };

    Playback<int> playback;
    if (!playback.load(recordShared<int>(AlgorithmType::QuickSort, data)))
        return 1;

    while (playback.getCurrentOperation() != nullptr)
    {
        const Operation<int> op = *playback.getCurrentOperation();
        playback.stepForward();
        if (op.type != OperationType::MarkSet && op.type != OperationType::MarkClear && op.type != OperationType::Swap)
            continue;

        std::printf("%-40s marks:", describe(op).c_str());
        if (playback.getMarks().empty())
            std::printf(" -");
        for (const auto& [id, index] : playback.getMarks())
            std::printf(" %s@[%zu]", markName(id).c_str(), static_cast<std::size_t>(index));
        std::printf("\n");
    }
    return 0;
}
