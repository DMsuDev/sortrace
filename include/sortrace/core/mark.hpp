// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include <compare>
#include <cstdint>
#include <string_view>

namespace sortrace {

/// What a mark means. Custom marks carry their own id (see MarkId::custom).
enum class MarkRole : std::uint8_t { Pivot, Minimum, Root, Active, Custom };

/// Identifies a mark. One element can carry several marks; each id points at one element at a time.
struct MarkId
{
    MarkRole      role   = MarkRole::Pivot;
    std::uint32_t custom = 0; // Only meaningful for MarkRole::Custom.

    constexpr MarkId() noexcept = default;
    constexpr MarkId(MarkRole r) noexcept : role(r) {} // NOLINT: implicit on purpose, `Step::mark(MarkRole::Pivot, i)`.

    [[nodiscard]] static constexpr MarkId custom_id(std::uint32_t id) noexcept
    {
        MarkId mark(MarkRole::Custom);
        mark.custom = id;
        return mark;
    }

    friend constexpr auto operator<=>(const MarkId&, const MarkId&) noexcept = default;
};

[[nodiscard]] constexpr std::string_view toString(MarkRole role) noexcept
{
    switch (role)
    {
        case MarkRole::Pivot:   return "Pivot";
        case MarkRole::Minimum: return "Minimum";
        case MarkRole::Root:    return "Root";
        case MarkRole::Active:  return "Active";
        case MarkRole::Custom:  return "Custom";
    }
    return "Unknown";
}

} // namespace sortrace
