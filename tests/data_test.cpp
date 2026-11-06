// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#include <sortrace/sortrace.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <random>

using namespace sortrace;

TEST(DataSetBuilder, BuildIsSortedAndSpansTheValueRange)
{
    const auto values = DataSetBuilder::build<int>({ 10, 2, 4096, 100 });

    ASSERT_EQ(values.size(), 10u);
    EXPECT_TRUE(std::ranges::is_sorted(values));
    EXPECT_EQ(values.front(), 1);
    EXPECT_EQ(values.back(), 100);
}

TEST(DataSetBuilder, CountIsClampedAndAnInvertedRangeFavoursTheMinimum)
{
    EXPECT_EQ(DataSetBuilder::build<int>({ 1, 2, 4096, 100 }).size(), 2u);
    EXPECT_EQ(DataSetBuilder::build<int>({ 500, 2, 64, 100 }).size(), 64u);
    EXPECT_EQ(DataSetBuilder::build<int>({ 10, 8, 4, 100 }).size(), 8u); // std::clamp would be UB here
}

TEST(DataSetBuilder, DegenerateConfigsStayValid)
{
    EXPECT_TRUE(DataSetBuilder::build<int>({ 0, 0, 10, 100 }).empty());
    EXPECT_EQ(DataSetBuilder::build<int>({ 4, 2, 4096, 0 }).back(), 1); // maxValue raised to 1
}

TEST(DataSetBuilder, WorksForFloatingPointValues)
{
    const auto values = DataSetBuilder::build<double>({ 5, 2, 100, 5 });
    EXPECT_EQ(values, (std::vector<double>{ 1.0, 2.0, 3.0, 4.0, 5.0 }));
}

TEST(DataSetBuilder, TheSameSeedGivesTheSameShuffle)
{
    const DataSetConfig config{ 32, 2, 4096, 100 };
    std::mt19937 first{ 7 }, second{ 7 };

    EXPECT_EQ(DataSetBuilder::buildShuffled<int>(config, first), DataSetBuilder::buildShuffled<int>(config, second));
}

TEST(SeedProvider, GlobalIsSharedAndPerSessionIsReproducibleAndDistinct)
{
    const auto global = SeedProvider::globalSeed(7);
    EXPECT_EQ(global.seedFor(0), global.seedFor(5));

    const auto a = SeedProvider::perSession(7);
    const auto b = SeedProvider::perSession(7);
    EXPECT_EQ(a.seedFor(3), b.seedFor(3));
    EXPECT_NE(a.seedFor(0), a.seedFor(1));
}
