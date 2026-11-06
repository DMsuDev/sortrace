// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/concepts.hpp"

#include <algorithm>
#include <random>
#include <type_traits>
#include <vector>

namespace sortrace {

/// How a test dataset is generated. Pure value object.
struct DataSetConfig
{
    int count    = 128;
    int minCount = 2;
    int maxCount = 4096;
    int maxValue = 500;

    [[nodiscard]] constexpr bool operator==(const DataSetConfig&) const noexcept = default;

    /// count clamped to [minCount, maxCount]. An inverted range favours minCount (std::clamp would be UB).
    [[nodiscard]] constexpr int effectiveCount() const noexcept
    {
        return std::clamp(count, minCount, std::max(minCount, maxCount));
    }

    /// At least 1, so the range [1, maxValue] is always valid.
    [[nodiscard]] constexpr int effectiveMaxValue() const noexcept { return std::max(maxValue, 1); }
};

/// Generates and shuffles datasets. build() is deterministic; shuffle() is the only user of the RNG.
/// Your own data does not need any of this: Session takes any std::vector<T>.
class DataSetBuilder
{
public:
    /// Sorted values evenly spread over [1, maxValue]. Empty if the clamped count is <= 0.
    template <class T>
        requires std::is_arithmetic_v<T> && SortableValue<T>
    [[nodiscard]] static std::vector<T> build(const DataSetConfig& config)
    {
        const int count    = config.effectiveCount();
        const int maxValue = config.effectiveMaxValue();
        if (count <= 0)
            return {};

        std::vector<T> values;
        values.reserve(static_cast<std::size_t>(count));

        if (count == 1)
        {
            values.push_back(static_cast<T>(maxValue));
            return values;
        }

        for (int i = 0; i < count; ++i)
        {
            if constexpr (std::is_integral_v<T>)
                values.push_back(static_cast<T>(1 + (static_cast<long long>(i) * (maxValue - 1)) / (count - 1)));
            else
                values.push_back(static_cast<T>(1.0 + (static_cast<double>(i) * (maxValue - 1)) / (count - 1)));
        }
        return values;
    }

    template <class T, class Rng>
    static void shuffle(std::vector<T>& values, Rng& rng)
    {
        std::shuffle(values.begin(), values.end(), rng);
    }

    template <class T, class Rng>
        requires std::is_arithmetic_v<T> && SortableValue<T>
    [[nodiscard]] static std::vector<T> buildShuffled(const DataSetConfig& config, Rng& rng)
    {
        auto values = build<T>(config);
        shuffle(values, rng);
        return values;
    }
};

} // namespace sortrace
