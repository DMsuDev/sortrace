// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <functional>

namespace sortrace {

/// A value that can be sorted: copyable and default-constructible. Ordering comes from a comparator.
template <class T>
concept SortableValue = std::semiregular<T>;

/// "a goes before b". Pass your own to sort by any key.
template <SortableValue T>
using Less = std::function<bool(const T&, const T&)>;

/// operator< as a Less<T>.
template <SortableValue T>
    requires std::totally_ordered<T>
[[nodiscard]] Less<T> defaultLess()
{
    return [](const T& a, const T& b) { return a < b; };
}

} // namespace sortrace
