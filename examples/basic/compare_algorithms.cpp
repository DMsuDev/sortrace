// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Compare: every built-in algorithm against several kinds of data.
// Shows that the shape of the input, not only its size, changes how much work an algorithm does.

#include "common/format.hpp"

#include <sortrace/sortrace.hpp>

#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using namespace sortrace;
using namespace sortrace::examples;

int main()
{
    constexpr int count = 200;
    auto          rng   = SeedProvider::globalSeed(5).createEngine(0);

    // Any std::vector works as input; DataSetBuilder is only a convenient way to make one.
    const auto ascending = DataSetBuilder::build<int>({ .count = count, .maxValue = count });

    auto descending = ascending;
    std::ranges::reverse(descending);

    auto nearlySorted = ascending; // A handful of neighbours out of place.
    for (std::size_t i = 0; i + 1 < nearlySorted.size(); i += 20)
        std::swap(nearlySorted[i], nearlySorted[i + 1]);

    auto fewUnique = DataSetBuilder::buildShuffled<int>({ .count = count, .maxValue = count }, rng);
    for (int& value : fewUnique)
        value = value % 4 + 1;

    const auto shuffled = DataSetBuilder::buildShuffled<int>({ .count = count, .maxValue = count }, rng);

    const std::vector<std::pair<std::string, const std::vector<int>*>> inputs = {
        { "ascending", &ascending },   { "descending", &descending }, { "nearly sorted", &nearlySorted },
        { "few unique", &fewUnique },  { "shuffled", &shuffled },
    };

    std::printf("Comparisons on %d values\n\n%-15s", count, "Algorithm");
    for (const auto& input : inputs)
        std::printf(" %14s", input.first.c_str());
    std::printf("\n");

    for (const AlgorithmType type : getAvailableAlgorithms())
    {
        std::printf("%-15s", std::string(toString(type)).c_str());
        for (const auto& input : inputs)
            std::printf(" %14s", thousands(record<int>(type, *input.second).statistics.comparisons).c_str());
        std::printf("\n");
    }
    return 0;
}
