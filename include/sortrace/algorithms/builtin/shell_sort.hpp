// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Shellsort
template <SortableValue T>
class ShellSort final : public GeneratorSort<T>
{
public:
    explicit ShellSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::ShellSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Insertion sort over elements `gap` apart, with the gap halving down to 1.
        const std::size_t n = d.size();
        for (std::size_t gap = n / 2; gap > 0; gap /= 2)
        {
            for (std::size_t i = gap; i < n; ++i)
            {
                for (std::size_t j = i; j >= gap; j -= gap)
                {
                    co_yield S::compare(j - gap, j);
                    if (!this->less(d[j], d[j - gap]))
                        break;
                    co_yield S::swap(j - gap, j);
                }
            }
        }
    }
};

} // namespace sortrace
