// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>
#include <vector>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Quicksort
template <SortableValue T>
class QuickSort final : public GeneratorSort<T>
{
public:
    explicit QuickSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::QuickSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Lomuto partition with a median-of-three pivot. The pivot is marked (MarkRole::Pivot) once it
        // sits at `high`, stays there during the partition, and the mark is cleared after the final swap.
        if (d.size() < 2)
            co_return;

        // Explicit stack: a coroutine cannot recurse into itself cheaply.
        std::vector<std::pair<std::size_t, std::size_t>> ranges{ { 0, d.size() - 1 } };

        while (!ranges.empty())
        {
            const auto range = ranges.back();
            const std::size_t low  = range.first;
            const std::size_t high = range.second;
            ranges.pop_back();

            if (high - low >= 2)
            {
                const std::size_t mid = low + (high - low) / 2;

                co_yield S::compare(low, mid);
                if (this->less(d[mid], d[low]))
                    co_yield S::swap(low, mid);

                co_yield S::compare(low, high);
                if (this->less(d[high], d[low]))
                    co_yield S::swap(low, high);

                co_yield S::compare(mid, high);
                if (this->less(d[high], d[mid]))
                    co_yield S::swap(mid, high);

                // low <= mid <= high: the median sits at mid. Park it at `high` as the pivot.
                co_yield S::swap(mid, high);
            }

            co_yield S::mark(MarkRole::Pivot, high);
            const T pivot = d[high];

            std::size_t boundary = low; // [low, boundary) holds elements smaller than the pivot.
            for (std::size_t j = low; j < high; ++j)
            {
                co_yield S::compare(j, high);
                if (this->less(d[j], pivot))
                {
                    if (boundary != j)
                        co_yield S::swap(boundary, j);
                    ++boundary;
                }
            }

            if (boundary != high)
                co_yield S::swap(boundary, high);
            co_yield S::clearMark(MarkRole::Pivot);

            // Right first, so the left range is popped first and the sort sweeps left to right.
            if (boundary + 1 < high)
                ranges.emplace_back(boundary + 1, high);
            if (boundary > low + 1)
                ranges.emplace_back(low, boundary - 1);
        }
    }
};

} // namespace sortrace
