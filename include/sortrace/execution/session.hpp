// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/algorithm.hpp"
#include "sortrace/core/log.hpp"
#include "sortrace/core/recording.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <expected>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace sortrace {

enum class ComputeError
{
    NoAlgorithm,         // setAlgorithm() was never called.
    NoData,              // setData() was never called, or the data is empty.
    TooLargeForBlocking, // Blocking run over more than SessionOptions::blockingLimit elements; pass a budget.
    AlgorithmThrew       // The algorithm threw; partial results were discarded.
};

enum class Progress
{
    InProgress, // The time budget ran out; call compute() again to continue.
    Complete    // getRecording() is ready.
};

[[nodiscard]] constexpr std::string_view toString(ComputeError error) noexcept
{
    switch (error)
    {
        case ComputeError::NoAlgorithm:         return "NoAlgorithm";
        case ComputeError::NoData:              return "NoData";
        case ComputeError::TooLargeForBlocking: return "TooLargeForBlocking";
        case ComputeError::AlgorithmThrew:      return "AlgorithmThrew";
    }
    return "Unknown";
}

struct SessionOptions
{
    /// Above this many elements, O(n²) algorithms can produce millions of operations, so a blocking
    /// compute() (no budget) is refused. Use a budget instead.
    std::size_t blockingLimit = 4096;
};

namespace detail {
inline std::uint32_t nextSessionId() noexcept
{
    static std::atomic<std::uint32_t> counter{ 0 };
    return counter.fetch_add(1, std::memory_order_relaxed);
}
} // namespace detail

/// One run of one algorithm over one dataset, computed in time slices so a UI never freezes.
/// Knows nothing about playback or rendering: when complete it hands out an immutable Recording.
/// Sessions are independent (no globals), so N of them can exist side by side.
template <SortableValue T>
class Session
{
public:
    explicit Session(SessionOptions options = {}) : m_Id(detail::nextSessionId()), m_Options(options) {}

    Session(Session&&) noexcept            = default;
    Session& operator=(Session&&) noexcept = default;
    Session(const Session&)                = delete;
    Session& operator=(const Session&)     = delete;

    void setLogSink(LogSink sink) { m_Logger.setSink(std::move(sink)); }

    /// Replaces the algorithm and discards any result.
    void setAlgorithm(std::unique_ptr<SortAlgorithm<T>> algorithm)
    {
        m_Algorithm = std::move(algorithm);
        clearOperations();
        m_Logger.log(LogLevel::Debug, "Session {}: algorithm set to {}", m_Id, getAlgorithmName());
    }

    /// Replaces the input data and discards any result.
    void setData(std::vector<T> data)
    {
        m_Data = std::move(data);
        clearOperations();
    }

    /// Runs the algorithm, recording every operation, for at most @p budget (checked every few hundred
    /// steps, so it can be slightly overshot; every call makes progress). Call again with the same
    /// budget to resume. A zero budget runs to completion and is refused above blockingLimit.
    [[nodiscard]] std::expected<Progress, ComputeError> compute(std::chrono::microseconds budget = std::chrono::microseconds::zero())
    {
        if (!m_Algorithm)
            return fail(ComputeError::NoAlgorithm);
        if (m_Data.empty())
            return fail(ComputeError::NoData);
        if (m_Result)
            return Progress::Complete;

        const bool unlimited = (budget == std::chrono::microseconds::zero());

        // Checked on every blocking call, also when force-completing a budgeted run in progress.
        if (unlimited && m_Data.size() > m_Options.blockingLimit)
        {
            m_Logger.log(LogLevel::Warn, "Session {}: {} elements exceed the blocking limit ({}); pass a budget",
                         m_Id, m_Data.size(), m_Options.blockingLimit);
            return fail(ComputeError::TooLargeForBlocking);
        }

        if (!m_InProgress)
        {
            m_Building           = Recording<T>{};
            m_Building.algorithm = std::string(m_Algorithm->getName());
            m_Building.initial   = m_Data;

            try
            {
                m_Algorithm->load(m_Data);
            }
            catch (const std::exception& e)
            {
                m_Logger.log(LogLevel::Error, "Session {}: '{}' threw while loading: {}", m_Id, getAlgorithmName(), e.what());
                clearOperations();
                return fail(ComputeError::AlgorithmThrew);
            }
            m_InProgress = true;
        }

        const auto t0 = std::chrono::steady_clock::now();

        try
        {
            Recorder<T> recorder(m_Building.statistics);
            for (std::size_t steps = 0;; ++steps)
            {
                // Never check on the first step, so a tiny budget still makes progress.
                if (!unlimited && steps != 0 && steps % kStepsPerClockCheck == 0 &&
                    (std::chrono::steady_clock::now() - t0) >= budget)
                    break;

                auto op = m_Algorithm->step(recorder);
                if (!op)
                {
                    m_InProgress = false;
                    break;
                }
                m_Building.operations.push_back(std::move(*op));
            }
        }
        catch (const std::exception& e)
        {
            m_Logger.log(LogLevel::Error, "Session {}: '{}' threw: {}. Partial results discarded.", m_Id, getAlgorithmName(), e.what());
            clearOperations();
            return fail(ComputeError::AlgorithmThrew);
        }

        m_Building.statistics.computeTimeMs +=
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

        if (m_InProgress)
            return Progress::InProgress;

        m_Logger.log(LogLevel::Info, "Session {}: {} operations in {:.2f} ms", m_Id, m_Building.operations.size(),
                     m_Building.statistics.computeTimeMs);
        m_Result   = std::make_shared<const Recording<T>>(std::move(m_Building));
        m_Building = Recording<T>{};
        return Progress::Complete;
    }

    /// Discards the result (or the run in progress) and frees its memory.
    void clearOperations()
    {
        m_Building   = Recording<T>{};
        m_Result.reset();
        m_InProgress = false;
    }

    [[nodiscard]] bool isComputing() const { return m_InProgress; }

    /// The finished recording, or null until compute() returns Complete. Shared and immutable.
    [[nodiscard]] std::shared_ptr<const Recording<T>> getRecording() const { return m_Result; }

    /// Final once complete; partial while computing.
    [[nodiscard]] const Statistics& getStatistics() const { return m_Result ? m_Result->statistics : m_Building.statistics; }

    [[nodiscard]] std::span<const T> getData() const { return m_Data; }
    [[nodiscard]] std::uint32_t getSessionId() const { return m_Id; }

    [[nodiscard]] std::string_view getAlgorithmName() const        { return m_Algorithm ? m_Algorithm->getName() : "None"; }
    [[nodiscard]] std::string_view getAlgorithmComplexity() const  { return m_Algorithm ? m_Algorithm->getComplexity() : "N/A"; }
    [[nodiscard]] std::string_view getAlgorithmDescription() const { return m_Algorithm ? m_Algorithm->getDescription() : "No algorithm selected."; }

private:
    // Reading the clock on every step would cost more than the cheap steps of the O(n²) algorithms.
    static constexpr std::size_t kStepsPerClockCheck = 256;

    std::unexpected<ComputeError> fail(ComputeError error) const
    {
        m_Logger.log(LogLevel::Debug, "Session {}: compute() failed: {}", m_Id, toString(error));
        return std::unexpected(error);
    }

    std::uint32_t                           m_Id;
    SessionOptions                          m_Options;
    Logger                                  m_Logger;
    std::unique_ptr<SortAlgorithm<T>>       m_Algorithm;
    std::vector<T>                          m_Data;
    Recording<T>                            m_Building;
    std::shared_ptr<const Recording<T>>     m_Result;
    bool                                    m_InProgress = false;
};

} // namespace sortrace
