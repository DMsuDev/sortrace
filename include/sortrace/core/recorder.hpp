// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/operation.hpp"
#include "sortrace/core/statistics.hpp"

#include <cassert>
#include <cstddef>
#include <span>
#include <utility>

// NOTE: Operation statistics are updated as follows:
//   compare:               comparisons +1, reads +2
//   swap:                  swaps +1, reads +2, writes +2
//   overwrite:             reads +1, writes +1
//   mark/clearMark/sorted: operationCount only

namespace sortrace {

/// @brief Records sorting operations while applying them to the underlying data.
///
/// Builds Operation objects from actual data modifications and updates the
/// associated statistics. Operations capture the values involved in each action,
/// ensuring the recording reflects what actually happened.
///
/// The recorder does not execute sorting algorithms; it only applies and
/// records the operations they produce.
///
/// @tparam T The type of values being sorted.
template <SortableValue T>
class Recorder
{
public:
    explicit Recorder(Statistics& statistics) : m_Statistics(statistics) {}

    [[nodiscard]] Operation<T> compare(std::span<const T> buffer, std::size_t i, std::size_t j)
    {
        assert(i < buffer.size() && j < buffer.size());
        ++m_Statistics.comparisons;
        m_Statistics.reads += 2;
        return record({ OperationType::Compare, toIndex(i), toIndex(j), buffer[i], buffer[j], {} });
    }

    [[nodiscard]] Operation<T> swap(std::span<T> buffer, std::size_t i, std::size_t j)
    {
        assert(i < buffer.size() && j < buffer.size());
        Operation<T> op{ OperationType::Swap, toIndex(i), toIndex(j), buffer[i], buffer[j], {} };
        using std::swap;
        swap(buffer[i], buffer[j]);
        ++m_Statistics.swaps;
        m_Statistics.reads += 2;
        m_Statistics.writes += 2;
        return record(std::move(op));
    }

    [[nodiscard]] Operation<T> overwrite(std::span<T> buffer, std::size_t index, T newValue)
    {
        assert(index < buffer.size());
        Operation<T> op{ OperationType::Overwrite, toIndex(index), -1, newValue, buffer[index], {} };
        buffer[index] = std::move(newValue);
        ++m_Statistics.reads;
        ++m_Statistics.writes;
        return record(std::move(op));
    }

    [[nodiscard]] Operation<T> mark(std::span<const T> buffer, MarkId id, std::size_t index)
    {
        assert(index < buffer.size());
        return record({ OperationType::MarkSet, toIndex(index), -1, buffer[index], {}, id });
    }

    [[nodiscard]] Operation<T> clearMark(MarkId id)
    {
        return record({ OperationType::MarkClear, -1, -1, {}, {}, id });
    }

    [[nodiscard]] Operation<T> sorted() { return record({ OperationType::Sorted, -1, -1, {}, {}, {} }); }

private:
    static constexpr int toIndex(std::size_t index) noexcept { return static_cast<int>(index); }

    Operation<T> record(Operation<T> op)
    {
        ++m_Statistics.operationCount;
        return op;
    }

    Statistics& m_Statistics;
};

} // namespace sortrace
