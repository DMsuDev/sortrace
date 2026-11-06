// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/step.hpp"

#include <coroutine>
#include <exception>
#include <utility>

namespace sortrace {

/// Minimal move-only generator of Steps (std::generator is not available everywhere).
/// It is the return type of every algorithm's run() coroutine.
template <SortableValue T>
class StepGenerator
{
public:
    struct promise_type
    {
        Step<T>            current;
        std::exception_ptr error;

        StepGenerator get_return_object() { return StepGenerator(std::coroutine_handle<promise_type>::from_promise(*this)); }
        std::suspend_always initial_suspend() noexcept { return {}; } // Lazy: nothing runs before next().
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(Step<T> step) { current = std::move(step); return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { error = std::current_exception(); }
    };

    StepGenerator() = default;
    StepGenerator(StepGenerator&& other) noexcept : m_Handle(std::exchange(other.m_Handle, {})) {}
    StepGenerator& operator=(StepGenerator&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_Handle = std::exchange(other.m_Handle, {});
        }
        return *this;
    }
    StepGenerator(const StepGenerator&)            = delete;
    StepGenerator& operator=(const StepGenerator&) = delete;
    ~StepGenerator() { reset(); }

    /// Runs to the next Step. False once there is nothing left. Rethrows what the algorithm threw.
    bool next()
    {
        if (!m_Handle || m_Handle.done())
            return false;

        m_Handle.resume();
        if (m_Handle.promise().error)
            std::rethrow_exception(m_Handle.promise().error);
        return !m_Handle.done();
    }

    /// The Step produced by the last successful next().
    [[nodiscard]] const Step<T>& value() const { return m_Handle.promise().current; }

private:
    explicit StepGenerator(std::coroutine_handle<promise_type> handle) : m_Handle(handle) {}

    void reset()
    {
        if (m_Handle)
            m_Handle.destroy();
        m_Handle = {};
    }

    std::coroutine_handle<promise_type> m_Handle;
};

} // namespace sortrace
