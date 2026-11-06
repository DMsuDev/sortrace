// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cctype>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sortrace {

/// Identifies a built-in algorithm. Values index the registry below (checked at compile time).
enum class AlgorithmType
{
    BubbleSort = 0,
    CocktailSort,
    SelectionSort,
    InsertionSort,
    QuickSort,
    MergeSort,
    HeapSort,
    ShellSort,

    Count // Keep last. Not an algorithm.
};

/// Static description of an algorithm (string literals, safe to keep).
struct AlgorithmMeta
{
    std::string_view name;
    std::string_view complexity; // Average case.
    std::string_view description;
};

struct AlgorithmInfo
{
    AlgorithmType    type;
    AlgorithmMeta    meta;
};

/// The registry. Adding an algorithm = one enumerator above + one line here, in the same order.
inline constexpr std::array<AlgorithmInfo, static_cast<std::size_t>(AlgorithmType::Count)> kAlgorithms = {{
    { AlgorithmType::BubbleSort,    { "Bubble Sort",    "O(n²)",      "Swaps adjacent out-of-order pairs; stops once a pass makes no swap." } },
    { AlgorithmType::CocktailSort,  { "Cocktail Sort",  "O(n²)",      "Bidirectional variation of Bubble Sort." } },
    { AlgorithmType::SelectionSort, { "Selection Sort", "O(n²)",      "Repeatedly selects the smallest remaining element and moves it into place." } },
    { AlgorithmType::InsertionSort, { "Insertion Sort", "O(n²)",      "Grows a sorted prefix by sliding each element into its place." } },
    { AlgorithmType::QuickSort,     { "Quick Sort",     "O(n log n)", "Partitions around a pivot (Lomuto scheme) and sorts each side." } },
    { AlgorithmType::MergeSort,     { "Merge Sort",     "O(n log n)", "Bottom-up merging of sorted runs through an auxiliary buffer." } },
    { AlgorithmType::HeapSort,      { "Heap Sort",      "O(n log n)", "Builds a max-heap, then repeatedly moves its root to the end." } },
    { AlgorithmType::ShellSort,     { "Shell Sort",     "O(n log² n)","Insertion sort over shrinking gaps, so distant elements move early." } },
}};

namespace detail {
constexpr bool registryMatchesEnum()
{
    for (std::size_t i = 0; i < kAlgorithms.size(); ++i)
        if (static_cast<std::size_t>(kAlgorithms[i].type) != i)
            return false;
    return true;
}
static_assert(registryMatchesEnum(), "kAlgorithms must list every AlgorithmType, in enum order.");
} // namespace detail

[[nodiscard]] constexpr std::span<const AlgorithmInfo> algorithms() noexcept { return kAlgorithms; }

/// Registry entry for @p type, or nullptr if it is not a built-in algorithm.
[[nodiscard]] constexpr const AlgorithmInfo* find(AlgorithmType type) noexcept
{
    const auto index = static_cast<std::size_t>(type);
    return index < kAlgorithms.size() ? &kAlgorithms[index] : nullptr;
}

[[nodiscard]] constexpr std::string_view toString(AlgorithmType type) noexcept
{
    const auto* info = find(type);
    return info ? info->meta.name : "Unknown Algorithm";
}

[[nodiscard]] inline std::vector<AlgorithmType> getAvailableAlgorithms()
{
    std::vector<AlgorithmType> types;
    types.reserve(kAlgorithms.size());
    for (const auto& info : kAlgorithms)
        types.push_back(info.type);
    return types;
}

/// "quick", "Quick Sort" and "quick_sort" all find QuickSort. Handy for command lines.
[[nodiscard]] inline std::optional<AlgorithmType> parseAlgorithm(std::string_view text)
{
    const auto normalize = [](std::string_view s) {
        std::string out;
        for (const char c : s)
            if (std::isalnum(static_cast<unsigned char>(c)))
                out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return out;
    };

    const std::string wanted = normalize(text);
    for (const auto& info : kAlgorithms)
    {
        const std::string full = normalize(info.meta.name);
        const std::string shortName = full.substr(0, full.size() - 4); // drops "sort"
        if (wanted == full || wanted == shortName)
            return info.type;
    }
    return std::nullopt;
}

} // namespace sortrace
