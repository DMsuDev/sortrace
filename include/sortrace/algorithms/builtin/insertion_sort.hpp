// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Insertion_sort
template <SortableValue T>
class InsertionSort final : public GeneratorSort<T>
{
public:
    explicit InsertionSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::InsertionSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Element i slides left, one swap at a time, until the one before it is not larger.
        const std::size_t n = d.size();
        for (std::size_t i = 1; i < n; ++i)
        {
            for (std::size_t j = i; j > 0; --j)
            {
                co_yield S::compare(j - 1, j);
                if (!this->less(d[j], d[j - 1]))
                    break;
                co_yield S::swap(j - 1, j);
            }
        }
    }
};

} // namespace sortrace
