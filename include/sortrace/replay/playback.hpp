// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/core/log.hpp"
#include "sortrace/core/recording.hpp"

#include <cstddef>
#include <expected>
#include <map>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace sortrace {

enum class PlaybackError
{
    EmptyRecording,   // No recording, or it has no operations.
    InvalidOperation  // An operation uses an index outside the data.
};

/// Replays a Recording over its own copy of the data, forward and backward. The recording is shared,
/// never copied. Time is optional: drive it with update(dt), or just call stepForward()/stepBackward().
///
/// State invariant: Stopped means "at the first operation, data untouched". Any advance leaves
/// Stopped; going back to the first operation returns to it.
template <SortableValue T>
class Playback
{
public:
    static constexpr float DEFAULT_STEP_DELAY_MS = 10.0f;

    enum class State { Stopped, Playing, Paused, Finished };

    /// A mark and the element it currently points at.
    using Marks = std::map<MarkId, int>;

    void setLogSink(LogSink sink) { m_Logger.setSink(std::move(sink)); }

    /// Starts from the recording's initial data, Stopped at the first operation. Every index is
    /// validated here, once, so replaying needs no checks. On error nothing changes.
    [[nodiscard]] std::expected<void, PlaybackError> load(std::shared_ptr<const Recording<T>> recording)
    {
        if (!recording || recording->operations.empty())
            return std::unexpected(PlaybackError::EmptyRecording);

        const std::size_t size = recording->initial.size();
        for (std::size_t i = 0; i < recording->operations.size(); ++i)
        {
            if (!isValid(recording->operations[i], size))
            {
                m_Logger.log(LogLevel::Error, "Playback: rejected recording, operation {} of {} uses an index outside the {} elements",
                             i, recording->operations.size(), size);
                return std::unexpected(PlaybackError::InvalidOperation);
            }
        }

        m_Recording = std::move(recording);
        buildSettledTimeline();
        reset();
        return {};
    }

    /// Takes ownership of a recording, e.g. `playback.load(record<int>(type, data))`. To share one
    /// recording between several playbacks, pass a shared_ptr instead.
    [[nodiscard]] std::expected<void, PlaybackError> load(Recording<T>&& recording)
    {
        return load(std::make_shared<const Recording<T>>(std::move(recording)));
    }

    /// Unloads everything. The playback becomes Stopped and empty.
    void clear()
    {
        m_Recording.reset();
        m_SettledAt.clear();
        reset();
    }

    /// Advances by elapsed time (seconds). Several steps may run in one call.
    void update(float deltaTime)
    {
        if (m_State != State::Playing)
            return;

        if (!m_DelayEnabled || m_DelayMs <= 0.0f)
        {
            while (stepForward()) {}
            return;
        }

        m_Elapsed += deltaTime;
        const float delaySeconds = m_DelayMs / 1000.0f;
        while (m_Elapsed >= delaySeconds)
        {
            if (!stepForward())
                break;
            m_Elapsed -= delaySeconds;
        }
    }

    /// Plays from the current position: starts from Stopped, resumes from Paused, restarts from Finished.
    /// @return False if there is nothing to play.
    bool play()
    {
        if (!isLoaded())
            return false;

        if (m_State == State::Finished)
            reset();

        m_Logger.log(LogLevel::Debug, "Playback: {} at operation {}", m_State == State::Paused ? "resumed" : "playing", m_Current);
        m_State = State::Playing;
        return true;
    }

    void pause()
    {
        if (m_State == State::Playing)
            m_State = State::Paused;
    }

    /// Back to the initial data and the first operation.
    void stop() { reset(); }

    /// Applies the next operation. @return False if there are none left.
    bool stepForward()
    {
        if (!isLoaded())
            return false;

        if (m_Current >= ops().size())
        {
            m_State = State::Finished;
            return false;
        }

        if (m_State == State::Stopped) // A manual step leaves the pristine state, so stepBackward() works.
            m_State = State::Paused;

        execute(ops()[m_Current], false);
        ++m_Current;

        if (m_Current >= ops().size())
            m_State = State::Finished;
        return true;
    }

    /// Undoes the previous operation and pauses. @return False if already at the first one.
    bool stepBackward()
    {
        if (!isLoaded() || m_Current == 0)
            return false;

        --m_Current;
        execute(ops()[m_Current], true);
        m_State = (m_Current == 0) ? State::Stopped : State::Paused;
        return true;
    }

    /// While disabled, update() applies every remaining operation at once. The delay is preserved.
    void enableDelay(bool value) { m_DelayEnabled = value; }
    [[nodiscard]] bool isDelayEnabled() const { return m_DelayEnabled; }

    void setDelayMs(float ms) { m_DelayMs = ms; }
    [[nodiscard]] float getDelayMs() const { return m_DelayMs; }

    [[nodiscard]] bool isLoaded() const { return m_Recording != nullptr; }

    /// True once no remaining operation changes this slot: it already holds its final value.
    /// Exact in both directions, and needs no help from the algorithms.
    [[nodiscard]] bool isSettled(std::size_t slot) const
    {
        return slot < m_SettledAt.size() && m_Current >= m_SettledAt[slot];
    }

    /// The data after every operation applied so far.
    [[nodiscard]] std::span<const T> getData() const { return m_Visual; }
    [[nodiscard]] std::span<const T> getInitialData() const
    {
        return m_Recording ? std::span<const T>(m_Recording->initial) : std::span<const T>{};
    }
    [[nodiscard]] std::span<const Operation<T>> getOperations() const
    {
        return m_Recording ? std::span<const Operation<T>>(m_Recording->operations) : std::span<const Operation<T>>{};
    }
    [[nodiscard]] const std::shared_ptr<const Recording<T>>& getRecording() const { return m_Recording; }

    /// The operation the next stepForward() would apply, or nullptr.
    [[nodiscard]] const Operation<T>* getCurrentOperation() const
    {
        return (isLoaded() && m_Current < ops().size()) ? &ops()[m_Current] : nullptr;
    }

    [[nodiscard]] std::size_t getCurrentOperationIndex() const { return m_Current; }
    [[nodiscard]] std::size_t getTotalOperations() const { return m_Recording ? ops().size() : 0; }

    /// Active marks. A mark is an element, not a slot: its index follows the element through swaps,
    /// in both directions.
    [[nodiscard]] const Marks& getMarks() const { return m_Marks; }

    /// Slot of mark @p id, or -1 if it is not set.
    [[nodiscard]] int getMarkIndex(MarkId id) const
    {
        const auto it = m_Marks.find(id);
        return it == m_Marks.end() ? -1 : it->second;
    }

    [[nodiscard]] State getState() const { return m_State; }

    /// In [0, 1]; 0 if nothing is loaded.
    [[nodiscard]] float getProgress() const
    {
        return getTotalOperations() == 0 ? 0.0f : static_cast<float>(m_Current) / static_cast<float>(getTotalOperations());
    }

    [[nodiscard]] bool isPlaying()  const { return m_State == State::Playing; }
    [[nodiscard]] bool isPaused()   const { return m_State == State::Paused; }
    [[nodiscard]] bool isStopped()  const { return m_State == State::Stopped; }
    [[nodiscard]] bool isFinished() const { return m_State == State::Finished; }

private:
    struct MarkMemento // What a Mark* operation replaced; index -1 = the mark did not exist.
    {
        MarkId id;
        int    index;
    };

    [[nodiscard]] const std::vector<Operation<T>>& ops() const { return m_Recording->operations; }

    static bool inRange(int index, std::size_t size) noexcept { return index >= 0 && static_cast<std::size_t>(index) < size; }

    static bool isValid(const Operation<T>& op, std::size_t size) noexcept
    {
        switch (op.type)
        {
            case OperationType::Swap:      return inRange(op.indexA, size) && inRange(op.indexB, size);
            case OperationType::Overwrite:
            case OperationType::MarkSet:   return inRange(op.indexA, size);
            case OperationType::Compare:
            case OperationType::MarkClear:
            case OperationType::Sorted:    return true; // Never touch the data.
        }
        return true;
    }

    void reset()
    {
        if (m_Recording)
            m_Visual = m_Recording->initial;
        else
            m_Visual.clear();
        m_Marks.clear();
        m_MarkHistory.clear();
        m_Current = 0;
        m_Elapsed = 0.0f;
        m_State   = State::Stopped;
    }

    void buildSettledTimeline()
    {
        m_SettledAt.assign(m_Recording->initial.size(), 0); // Untouched slots are settled from the start.
        for (std::size_t i = 0; i < ops().size(); ++i)
        {
            const Operation<T>& op = ops()[i];
            if (op.type == OperationType::Swap)
            {
                m_SettledAt[static_cast<std::size_t>(op.indexA)] = i + 1;
                m_SettledAt[static_cast<std::size_t>(op.indexB)] = i + 1;
            }
            else if (op.type == OperationType::Overwrite)
            {
                m_SettledAt[static_cast<std::size_t>(op.indexA)] = i + 1;
            }
        }
    }

    void execute(const Operation<T>& op, bool reverse)
    {
        switch (op.type)
        {
            case OperationType::Swap:
            {
                // A swap is its own inverse. Marks ride on their element.
                std::swap(m_Visual[static_cast<std::size_t>(op.indexA)], m_Visual[static_cast<std::size_t>(op.indexB)]);
                for (auto& [id, index] : m_Marks)
                {
                    if (index == op.indexA)      index = op.indexB;
                    else if (index == op.indexB) index = op.indexA;
                }
                break;
            }
            case OperationType::Overwrite:
                m_Visual[static_cast<std::size_t>(op.indexA)] = reverse ? op.valueB : op.valueA;
                break;

            case OperationType::MarkSet:
            case OperationType::MarkClear:
                reverse ? undoMark() : applyMark(op);
                break;

            case OperationType::Compare:
            case OperationType::Sorted:
                break;
        }
    }

    void applyMark(const Operation<T>& op)
    {
        const auto it = m_Marks.find(op.mark);
        m_MarkHistory.push_back({ op.mark, it == m_Marks.end() ? -1 : it->second });

        if (op.type == OperationType::MarkSet)
            m_Marks[op.mark] = op.indexA;
        else if (it != m_Marks.end())
            m_Marks.erase(it);
    }

    void undoMark()
    {
        // Undo runs in LIFO order, so the history always has the matching entry.
        const MarkMemento previous = m_MarkHistory.back();
        m_MarkHistory.pop_back();

        if (previous.index < 0)
            m_Marks.erase(previous.id);
        else
            m_Marks[previous.id] = previous.index;
    }

    Logger                              m_Logger;
    std::shared_ptr<const Recording<T>> m_Recording;
    std::vector<T>                      m_Visual;
    Marks                               m_Marks;
    std::vector<MarkMemento>            m_MarkHistory;
    std::vector<std::size_t>            m_SettledAt; // Per slot: operations that must run before it stops changing.

    std::size_t m_Current = 0;
    float       m_Elapsed = 0.0f;
    float       m_DelayMs = DEFAULT_STEP_DELAY_MS;
    bool        m_DelayEnabled = true;
    State       m_State = State::Stopped;
};

} // namespace sortrace
