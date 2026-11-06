// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#include <sortrace/sortrace.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

using namespace sortrace;

namespace {

std::vector<std::vector<int>> standardInputs()
{
    return {
        {}, {1}, {2, 1}, {1, 2, 3, 4, 5}, {5, 4, 3, 2, 1},
        {3, 1, 3, 2, 1, 2}, // duplicates
        {7, 7, 7, 7},
        {9, 2, 7, 4, 5, 6, 3, 8, 1, 10, 0, -4},
    };
}

std::string paramName(const ::testing::TestParamInfo<AlgorithmType>& info)
{
    std::string name{ toString(info.param) };
    std::erase(name, ' ');
    return name;
}

class AlgorithmTest : public ::testing::TestWithParam<AlgorithmType> {};

} // namespace

TEST_P(AlgorithmTest, SortsEveryStandardInput)
{
    for (const auto& input : standardInputs())
    {
        SCOPED_TRACE(::testing::PrintToString(input));

        auto algorithm = make<int>(GetParam());
        ASSERT_NE(algorithm, nullptr);
        (void)record<int>(*algorithm, input);

        const auto result = algorithm->getData();
        EXPECT_TRUE(std::ranges::is_sorted(result));
        EXPECT_TRUE(std::ranges::is_permutation(result, input));
    }
}

TEST_P(AlgorithmTest, SortsByAnyComparatorAndAnyValueType)
{
    auto descending = make<int>(GetParam(), std::greater<int>{});
    const std::vector<int> numbers{ 4, 9, 1, 7, 3, 8 };
    (void)record<int>(*descending, numbers);
    EXPECT_TRUE(std::ranges::is_sorted(descending->getData(), std::greater<int>{}));

    auto strings = make<std::string>(GetParam());
    const std::vector<std::string> words{ "pear", "fig", "apple", "kiwi", "date" };
    (void)record<std::string>(*strings, words);
    EXPECT_TRUE(std::ranges::is_sorted(strings->getData()));
}

TEST_P(AlgorithmTest, EndsWithExactlyOneSortedOperation)
{
    for (const auto& input : standardInputs())
    {
        SCOPED_TRACE(::testing::PrintToString(input));

        auto algorithm = make<int>(GetParam());
        const auto recording = record<int>(*algorithm, input);

        ASSERT_FALSE(recording.operations.empty());
        EXPECT_EQ(recording.operations.back().type, OperationType::Sorted);
        EXPECT_EQ(std::ranges::count(recording.operations, OperationType::Sorted, &Operation<int>::type), 1);

        Statistics unused;
        Recorder<int> recorder(unused);
        EXPECT_FALSE(algorithm->step(recorder).has_value()); // nothing after Sorted
    }
}

TEST_P(AlgorithmTest, StatisticsMatchTheRecordedOperations)
{
    const auto recording = record<int>(GetParam(), std::vector<int>{ 5, 3, 4, 1, 2 });
    const auto countOf = [&](OperationType type) {
        return static_cast<std::uint64_t>(std::ranges::count(recording.operations, type, &Operation<int>::type));
    };

    EXPECT_EQ(recording.statistics.operationCount, recording.operations.size());
    EXPECT_EQ(recording.statistics.comparisons, countOf(OperationType::Compare));
    EXPECT_EQ(recording.statistics.swaps, countOf(OperationType::Swap));
}

// The recording alone reproduces the result forward and the input backward.
TEST_P(AlgorithmTest, ReplaysForwardAndBackward)
{
    for (const auto& input : standardInputs())
    {
        SCOPED_TRACE(::testing::PrintToString(input));

        auto algorithm = make<int>(GetParam());
        auto recording = std::make_shared<const Recording<int>>(record<int>(*algorithm, input));

        Playback<int> playback;
        ASSERT_TRUE(playback.load(recording).has_value());

        while (playback.stepForward()) {}
        EXPECT_TRUE(std::ranges::equal(playback.getData(), algorithm->getData()));

        while (playback.stepBackward()) {}
        EXPECT_TRUE(std::ranges::equal(playback.getData(), input));
        EXPECT_TRUE(playback.isStopped());
    }
}

// A mark always sits on the element it was set on, and no run leaves one behind.
TEST_P(AlgorithmTest, MarksFollowTheirElementAndAreAllCleared)
{
    for (const auto& input : standardInputs())
    {
        SCOPED_TRACE(::testing::PrintToString(input));

        auto recording = std::make_shared<const Recording<int>>(record<int>(GetParam(), input));
        Playback<int> playback;
        ASSERT_TRUE(playback.load(recording).has_value());

        std::map<MarkId, int> markedValue;
        while (playback.stepForward())
        {
            const auto& applied = recording->operations[playback.getCurrentOperationIndex() - 1];
            if (applied.type == OperationType::MarkSet)
                markedValue[applied.mark] = applied.valueA;

            for (const auto& [id, index] : playback.getMarks())
            {
                ASSERT_LT(static_cast<std::size_t>(index), input.size());
                EXPECT_EQ(playback.getData()[static_cast<std::size_t>(index)], markedValue[id]);
            }
        }
        EXPECT_TRUE(playback.getMarks().empty());
    }
}

// A slot reported as settled already holds its final value, at every step.
TEST_P(AlgorithmTest, SettledSlotsAlreadyHoldTheirFinalValue)
{
    for (const auto& input : standardInputs())
    {
        SCOPED_TRACE(::testing::PrintToString(input));

        auto algorithm = make<int>(GetParam());
        auto recording = std::make_shared<const Recording<int>>(record<int>(*algorithm, input));
        const std::vector<int> finalData(algorithm->getData().begin(), algorithm->getData().end());

        Playback<int> playback;
        ASSERT_TRUE(playback.load(recording).has_value());

        do
        {
            for (std::size_t slot = 0; slot < input.size(); ++slot)
            {
                if (playback.isSettled(slot))
                {
                    EXPECT_EQ(playback.getData()[slot], finalData[slot]);
                }
            }
        } while (playback.stepForward());

        for (std::size_t slot = 0; slot < input.size(); ++slot)
            EXPECT_TRUE(playback.isSettled(slot));
    }
}

INSTANTIATE_TEST_SUITE_P(AllAlgorithms, AlgorithmTest, ::testing::ValuesIn(getAvailableAlgorithms()), paramName);

TEST(AlgorithmRegistry, EveryTypeCreatesItsAlgorithmAndParsesByName)
{
    for (const auto& info : algorithms())
    {
        const auto algorithm = make<int>(info.type);
        ASSERT_NE(algorithm, nullptr);
        EXPECT_EQ(algorithm->getName(), info.meta.name);
        EXPECT_EQ(parseAlgorithm(info.meta.name), info.type);
    }

    EXPECT_EQ(parseAlgorithm("quick_sort"), AlgorithmType::QuickSort);
    EXPECT_EQ(parseAlgorithm("HEAP"), AlgorithmType::HeapSort);
    EXPECT_FALSE(parseAlgorithm("bogo").has_value());
    EXPECT_EQ(make<int>(AlgorithmType::Count), nullptr);
}
