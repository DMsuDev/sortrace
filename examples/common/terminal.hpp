// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// Terminal helpers shared by the examples: ANSI codes and the only platform-specific code.
#pragma once

#include <cstdio>

#if defined(_WIN32)
  #include <conio.h>
  #include <io.h>
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <poll.h>
  #include <termios.h>
  #include <unistd.h>
#endif

namespace sortrace::examples {

namespace ansi {
inline constexpr const char* reset   = "\x1b[0m";
inline constexpr const char* bold    = "\x1b[1m";
inline constexpr const char* dim     = "\x1b[90m";
inline constexpr const char* red     = "\x1b[91m";
inline constexpr const char* green   = "\x1b[92m";
inline constexpr const char* yellow  = "\x1b[93m";
inline constexpr const char* blue    = "\x1b[94m";
inline constexpr const char* magenta = "\x1b[95m";
inline constexpr const char* cyan    = "\x1b[96m";
} // namespace ansi

[[nodiscard]] inline bool isTerminal()
{
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

// Windows consoles need UTF-8 and ANSI processing switched on. @return false if ANSI is not available.
inline bool setupTerminal()
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    const HANDLE out  = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD        mode = 0;
    return GetConsoleMode(out, &mode) && SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    return true;
#endif
}

[[nodiscard]] inline bool isInteractiveInput()
{
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

// ---------------------------------------------------------------------------------------------------
//  Single key presses (no Enter needed)
// ---------------------------------------------------------------------------------------------------

enum class Key { Next, Back, JumpForward, JumpBack, Start, End, Quit, Other, Closed };

// Puts the terminal in "one key at a time" mode while it lives (a no-op on Windows).
class RawInput
{
public:
    RawInput()
    {
#if !defined(_WIN32)
        if (tcgetattr(STDIN_FILENO, &m_Saved) == 0)
        {
            termios raw = m_Saved;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN]  = 1;
            raw.c_cc[VTIME] = 0;
            m_Active = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
#endif
    }

    ~RawInput()
    {
#if !defined(_WIN32)
        if (m_Active)
            tcsetattr(STDIN_FILENO, TCSANOW, &m_Saved);
#endif
    }

    RawInput(const RawInput&)            = delete;
    RawInput& operator=(const RawInput&) = delete;

private:
#if !defined(_WIN32)
    termios m_Saved{};
    bool    m_Active = false;
#endif
};

namespace detail {

inline Key keyFromChar(int c)
{
    switch (c)
    {
        case ' ': case '\r': case '\n': case 'n': case 'N': return Key::Next;
        case 'b': case 'B':                                 return Key::Back;
        case 'r': case 'R':                                 return Key::Start;
        case 'e': case 'E':                                 return Key::End;
        case 'q': case 'Q': case 3 /* Ctrl+C */:            return Key::Quit;
        default:                                            return Key::Other;
    }
}

#if !defined(_WIN32)
inline int readByte(int timeoutMs)
{
    pollfd fd{ STDIN_FILENO, POLLIN, 0 };
    if (poll(&fd, 1, timeoutMs) <= 0)
        return -1;
    unsigned char c = 0;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}
#endif

} // namespace detail

// Arrows move one step, Up/Down (or PageUp/PageDown) ten, Home/End go to the ends.
// Call it while a RawInput is alive.
[[nodiscard]] inline Key readKey()
{
#if defined(_WIN32)
    const int c = _getch();
    if (c == 0 || c == 0xE0) // Special key: a second code says which.
    {
        switch (_getch())
        {
            case 77: return Key::Next;        // Right
            case 75: return Key::Back;        // Left
            case 72: case 73: return Key::JumpForward; // Up, PageUp
            case 80: case 81: return Key::JumpBack;    // Down, PageDown
            case 71: return Key::Start;       // Home
            case 79: return Key::End;         // End
            default: return Key::Other;
        }
    }
    return detail::keyFromChar(c);
#else
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return Key::Closed;
    if (c != 27)
        return detail::keyFromChar(c);

    const int second = detail::readByte(50);
    if (second < 0)
        return Key::Quit; // A lone Esc.
    if (second != '[' && second != 'O')
        return Key::Other;

    const int code = detail::readByte(50);
    switch (code)
    {
        case 'C': return Key::Next;
        case 'D': return Key::Back;
        case 'A': return Key::JumpForward;
        case 'B': return Key::JumpBack;
        case 'H': return Key::Start;
        case 'F': return Key::End;
        case '1': case '7': case '4': case '8': case '5': case '6':
        {
            detail::readByte(50); // The closing '~'.
            if (code == '1' || code == '7') return Key::Start;
            if (code == '4' || code == '8') return Key::End;
            return code == '5' ? Key::JumpForward : Key::JumpBack;
        }
        default: return Key::Other;
    }
#endif
}

} // namespace sortrace::examples
