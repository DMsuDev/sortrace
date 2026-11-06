// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <algorithm>
#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Merge_sort
template <SortableValue T>
class MergeSort final : public GeneratorSort<T>
{
public:
    explicit MergeSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::MergeSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // Bottom-up: merge runs of width 1, 2, 4, ... Each merge reads a snapshot in m_Aux and writes
        // the result back into m_Data. The snapshot copy itself is not yielded, so it is not counted.
        auto& aux = this->m_Aux;
        const std::size_t n = d.size();

        for (std::size_t width = 1; width < n; width *= 2)
        {
            for (std::size_t low = 0; low + width < n; low += 2 * width)
            {
                const std::size_t mid  = low + width;
                const std::size_t high = std::min(low + 2 * width, n);

                for (std::size_t k = low; k < high; ++k)
                    aux[k] = d[k];

                std::size_t left = low, right = mid, out = low;
                while (left < mid && right < high)
                {
                    co_yield S::compareAux(left, right);
                    if (!this->less(aux[right], aux[left]))
                        co_yield S::overwrite(out++, aux[left++]);
                    else
                        co_yield S::overwrite(out++, aux[right++]);
                }
                while (left < mid)
                    co_yield S::overwrite(out++, aux[left++]);
                while (right < high)
                    co_yield S::overwrite(out++, aux[right++]);
            }
        }
    }
};

} // namespace sortrace
