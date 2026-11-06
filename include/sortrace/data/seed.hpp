// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace sortrace {

/// Where the random engine of each run comes from: one shared seed, a reproducible seed per
/// session index, or true randomness.
class SeedProvider
{
public:
    enum class Mode { GlobalSeed, PerSessionSeed, Random };

    [[nodiscard]] static SeedProvider globalSeed(std::uint32_t seed) { return { Mode::GlobalSeed, seed }; }
    [[nodiscard]] static SeedProvider perSession(std::uint32_t baseSeed) { return { Mode::PerSessionSeed, baseSeed }; }
    [[nodiscard]] static SeedProvider random() { return { Mode::Random, std::random_device{}() }; }

    /// The seed for the given session index.
    [[nodiscard]] std::uint32_t seedFor(std::size_t sessionIndex) const
    {
        switch (m_Mode)
        {
            case Mode::GlobalSeed:
                return m_BaseSeed;
            case Mode::PerSessionSeed:
            {
                std::seed_seq seq{ m_BaseSeed, static_cast<std::uint32_t>(sessionIndex) };
                std::uint32_t derived = 0;
                seq.generate(&derived, &derived + 1);
                return derived;
            }
            case Mode::Random:
                return std::random_device{}();
        }
        return m_BaseSeed;
    }

    [[nodiscard]] std::mt19937 createEngine(std::size_t sessionIndex) const { return std::mt19937{ seedFor(sessionIndex) }; }

private:
    SeedProvider(Mode mode, std::uint32_t baseSeed) : m_Mode(mode), m_BaseSeed(baseSeed) {}

    Mode          m_Mode;
    std::uint32_t m_BaseSeed;
};

} // namespace sortrace
