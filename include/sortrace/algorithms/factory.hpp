// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/algorithm.hpp"
#include "sortrace/algorithms/registry.hpp"
#include "sortrace/algorithms/builtin/bubble_sort.hpp"
#include "sortrace/algorithms/builtin/cocktail_sort.hpp"
#include "sortrace/algorithms/builtin/heap_sort.hpp"
#include "sortrace/algorithms/builtin/insertion_sort.hpp"
#include "sortrace/algorithms/builtin/merge_sort.hpp"
#include "sortrace/algorithms/builtin/quick_sort.hpp"
#include "sortrace/algorithms/builtin/selection_sort.hpp"
#include "sortrace/algorithms/builtin/shell_sort.hpp"

#include <memory>
#include <utility>

namespace sortrace {

/// New instance of a built-in algorithm ordering by @p less, or nullptr if @p type is unknown.
template <SortableValue T>
[[nodiscard]] std::unique_ptr<SortAlgorithm<T>> make(AlgorithmType type, Less<T> less)
{
    switch (type)
    {
        case AlgorithmType::BubbleSort:    return std::make_unique<BubbleSort<T>>(std::move(less));
        case AlgorithmType::CocktailSort:  return std::make_unique<CocktailSort<T>>(std::move(less));
        case AlgorithmType::SelectionSort: return std::make_unique<SelectionSort<T>>(std::move(less));
        case AlgorithmType::InsertionSort: return std::make_unique<InsertionSort<T>>(std::move(less));
        case AlgorithmType::QuickSort:     return std::make_unique<QuickSort<T>>(std::move(less));
        case AlgorithmType::MergeSort:     return std::make_unique<MergeSort<T>>(std::move(less));
        case AlgorithmType::HeapSort:      return std::make_unique<HeapSort<T>>(std::move(less));
        case AlgorithmType::ShellSort:     return std::make_unique<ShellSort<T>>(std::move(less));
        case AlgorithmType::Count:         break;
    }
    return nullptr;
}

/// Same, ordering by operator<.
template <SortableValue T>
    requires std::totally_ordered<T>
[[nodiscard]] std::unique_ptr<SortAlgorithm<T>> make(AlgorithmType type)
{
    return make<T>(type, defaultLess<T>());
}

} // namespace sortrace
