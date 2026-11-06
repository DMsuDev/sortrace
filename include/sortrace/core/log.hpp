// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include <format>
#include <functional>
#include <string_view>
#include <utility>

namespace sortrace {

enum class LogLevel { Trace, Debug, Info, Warn, Error };

/// Receives the library's diagnostics. Without a sink the library is silent.
using LogSink = std::function<void(LogLevel, std::string_view)>;

/// Per-object logger. Messages are only formatted when a sink is set.
class Logger
{
public:
    void setSink(LogSink sink) { m_Sink = std::move(sink); }

    template <class... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) const
    {
        if (m_Sink)
            m_Sink(level, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    LogSink m_Sink;
};

} // namespace sortrace
