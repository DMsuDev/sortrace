// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/generator_sort.hpp"

#include <utility>

namespace sortrace {

/// See https://en.wikipedia.org/wiki/Heapsort
template <SortableValue T>
class HeapSort final : public GeneratorSort<T>
{
public:
    explicit HeapSort(Less<T> less) : GeneratorSort<T>(find(AlgorithmType::HeapSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;
        // One loop for both phases: heapify (start walks from the last parent to the root), then
        // repeatedly swap the root behind the shrinking heap. [heapEnd, n) is already sorted.
        std::size_t heapEnd = d.size();
        std::size_t start   = heapEnd / 2;

        while (heapEnd > 1)
        {
            if (start > 0)
            {
                --start;
            }
            else
            {
                --heapEnd;
                co_yield S::swap(0, heapEnd);
            }

            std::size_t root = start;
            while (2 * root + 1 < heapEnd)
            {
                std::size_t child = 2 * root + 1;
                if (child + 1 < heapEnd)
                {
                    co_yield S::compare(child, child + 1);
                    if (this->less(d[child], d[child + 1]))
                        ++child;
                }

                co_yield S::compare(root, child);
                if (!this->less(d[root], d[child]))
                    break;

                co_yield S::swap(root, child);
                root = child;
            }
        }
    }
};

} // namespace sortrace
