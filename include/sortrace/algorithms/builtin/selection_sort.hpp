// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Selection_sort
template <SortableValue T>
class SelectionSort final : public GeneratorSort<T>
{
public:
    explicit SelectionSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::SelectionSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Marks the smallest element found so far (MarkRole::Minimum); at most one swap per pass.
        const std::size_t n = d.size();
        for (std::size_t i = 0; i + 1 < n; ++i)
        {
            std::size_t smallest = i;
            co_yield S::mark(MarkRole::Minimum, smallest);

            for (std::size_t j = i + 1; j < n; ++j)
            {
                co_yield S::compare(smallest, j);
                if (this->less(d[j], d[smallest]))
                {
                    smallest = j;
                    co_yield S::mark(MarkRole::Minimum, smallest);
                }
            }

            if (smallest != i)
                co_yield S::swap(i, smallest);
            co_yield S::clearMark(MarkRole::Minimum);
        }
    }
};

} // namespace sortrace
