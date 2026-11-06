// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Bubble_sort
template <SortableValue T>
class BubbleSort final : public GeneratorSort<T>
{
public:
    explicit BubbleSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::BubbleSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Every comparison is yielded, swap or not. A pass without swaps proves the data is sorted.
        for (std::size_t end = d.size(); end > 1; --end)
        {
            bool swapped = false;
            for (std::size_t i = 0; i + 1 < end; ++i)
            {
                co_yield S::compare(i, i + 1);
                if (this->less(d[i + 1], d[i]))
                {
                    co_yield S::swap(i, i + 1);
                    swapped = true;
                }
            }
            if (!swapped)
                break;
        }
    }
};

} // namespace sortrace
