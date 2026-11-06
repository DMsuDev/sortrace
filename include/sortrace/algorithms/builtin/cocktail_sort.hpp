// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Cocktail_shaker_sort
template <SortableValue T>
class CocktailSort final : public GeneratorSort<T>
{
public:
    explicit CocktailSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::CocktailSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Bubble Sort in both directions; the unsorted range shrinks from both ends.
        if (d.size() < 2)
            co_return;

        std::size_t start = 0;
        std::size_t end   = d.size() - 1;

        while (start < end)
        {
            bool swapped = false;
            for (std::size_t i = start; i < end; ++i)
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
            --end;

            swapped = false;
            for (std::size_t i = end; i > start; --i)
            {
                co_yield S::compare(i - 1, i);
                if (this->less(d[i], d[i - 1]))
                {
                    co_yield S::swap(i - 1, i);
                    swapped = true;
                }
            }
            if (!swapped)
                break;
            ++start;
        }
    }
};

} // namespace sortrace
