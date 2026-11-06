// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/algorithm.hpp"
#include "sortrace/algorithms/factory.hpp"
#include "sortrace/algorithms/registry.hpp"
#include "sortrace/core/concepts.hpp"
#include "sortrace/core/recording.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <span>
#include <utility>

namespace sortrace {

/// @brief Executes a sorting algorithm to completion and records its execution.
///
/// Runs @p algorithm over @p data, collecting every operation and the resulting
/// statistics into a Recording. This is a blocking, headless operation intended
/// for command-line tools, tests, and other non-interactive use cases.
///
/// For budgeted, incremental computation, use Session instead.
///
/// @param algorithm The sorting algorithm to execute.
/// @param data The input sequence to sort.
/// @return A recording containing the input, generated operations, and statistics.
/// @throws Any exception propagated by the algorithm.
template <SortableValue T>
[[nodiscard]] Recording<T> record(SortAlgorithm<T>& algorithm, std::span<const T> data)
{
    const auto t0 = std::chrono::steady_clock::now();

    Recording<T> recording;
    recording.algorithm = std::string(algorithm.getName());
    recording.initial.assign(data.begin(), data.end());

    Recorder<T> recorder(recording.statistics);
    algorithm.load(data);
    while (auto op = algorithm.step(recorder))
        recording.operations.push_back(std::move(*op));

    recording.statistics.computeTimeMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return recording;
}

/// Same, with a built-in algorithm: `record<int>(AlgorithmType::QuickSort, values)`.
/// Throws std::invalid_argument for an unknown type.
template <SortableValue T>
[[nodiscard]] Recording<T> record(AlgorithmType type, std::span<const T> data, Less<T> less)
{
    auto algorithm = make<T>(type, std::move(less));
    if (!algorithm)
        throw std::invalid_argument("sortrace::record: unknown algorithm type");
    return record<T>(*algorithm, data);
}

template <SortableValue T>
    requires std::totally_ordered<T>
[[nodiscard]] Recording<T> record(AlgorithmType type, std::span<const T> data)
{
    return record<T>(type, data, defaultLess<T>());
}

/// Records like `record()` and returns the result ready to share: pass it to several Playbacks (one per
/// view) and they all read the same operations. Accepts every argument list record() accepts.
/// `auto shared = recordShared<int>(AlgorithmType::QuickSort, values);`
template <SortableValue T, typename... Args>
[[nodiscard]] std::shared_ptr<const Recording<T>> recordShared(Args&&... args)
{
    return std::make_shared<const Recording<T>>(record<T>(std::forward<Args>(args)...));
}

} // namespace sortrace
