// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#include <sortrace/sortrace.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace sortrace;

namespace {

using Rec = Recording<int>;

// Three operations over {3, 1, 2}:  swap(0,1) -> {1,3,2},  swap(1,2) -> {1,2,3},  sorted.
std::shared_ptr<const Rec> swapRecording()
{
    auto rec = std::make_shared<Rec>();
    rec->initial = { 3, 1, 2 };
    std::vector<int> work = rec->initial;
    Recorder<int> recorder(rec->statistics);
    rec->operations = { recorder.swap(work, 0, 1), recorder.swap(work, 1, 2), recorder.sorted() };
    return rec;
}

class PlaybackTest : public ::testing::Test
{
protected:
    void SetUp() override { ASSERT_TRUE(playback.load(swapRecording()).has_value()); }

    static std::vector<int> v(std::initializer_list<int> values) { return values; }
    std::vector<int> data() const { return { playback.getData().begin(), playback.getData().end() }; }

    Playback<int> playback;
};

} // namespace

TEST_F(PlaybackTest, LoadLeavesItStoppedOverTheInitialData)
{
    EXPECT_TRUE(playback.isLoaded());
    EXPECT_TRUE(playback.isStopped());
    EXPECT_EQ(data(), v({ 3, 1, 2 }));
    EXPECT_EQ(playback.getTotalOperations(), 3u);
}

TEST(Playback, LoadRejectsBadRecordingsAndKeepsThePreviousOne)
{
    Playback<int> playback;
    EXPECT_EQ(playback.load(nullptr).error(), PlaybackError::EmptyRecording);
    EXPECT_EQ(playback.load(std::make_shared<Rec>()).error(), PlaybackError::EmptyRecording);

    ASSERT_TRUE(playback.load(swapRecording()).has_value());

    auto bad = std::make_shared<Rec>();
    bad->initial = { 1, 2, 3 };
    bad->operations.push_back({ OperationType::Swap, 0, 7, 0, 0, {} });
    EXPECT_EQ(playback.load(bad).error(), PlaybackError::InvalidOperation);

    bad->operations.back() = { OperationType::MarkSet, 3, -1, 0, 0, MarkRole::Pivot };
    EXPECT_EQ(playback.load(bad).error(), PlaybackError::InvalidOperation);

    EXPECT_EQ(playback.getTotalOperations(), 3u); // still the first recording
}

TEST_F(PlaybackTest, UpdateAppliesOneStepPerDelay)
{
    playback.setDelayMs(10.0f);
    playback.play();
    playback.update(0.025f);
    EXPECT_EQ(playback.getCurrentOperationIndex(), 2u);
}

TEST_F(PlaybackTest, DisablingTheDelayFinishesInOneUpdateAndKeepsTheDelay)
{
    playback.enableDelay(false);
    playback.play();
    playback.update(0.0f);

    EXPECT_TRUE(playback.isFinished());
    EXPECT_EQ(data(), v({ 1, 2, 3 }));
    EXPECT_FLOAT_EQ(playback.getDelayMs(), Playback<int>::DEFAULT_STEP_DELAY_MS);
}

TEST_F(PlaybackTest, PlayResumesFromPauseAndRestartsFromFinished)
{
    playback.setDelayMs(10.0f);
    playback.play();
    playback.update(0.015f);
    playback.pause();
    ASSERT_TRUE(playback.isPaused());

    playback.play(); // resume: position kept
    EXPECT_TRUE(playback.isPlaying());
    EXPECT_EQ(playback.getCurrentOperationIndex(), 1u);

    playback.update(1.0f);
    ASSERT_TRUE(playback.isFinished());

    playback.play(); // restart from the initial data
    EXPECT_TRUE(playback.isPlaying());
    EXPECT_EQ(playback.getCurrentOperationIndex(), 0u);
    EXPECT_EQ(data(), v({ 3, 1, 2 }));
}

TEST_F(PlaybackTest, StopRestoresTheDataAndTheCursor)
{
    playback.setDelayMs(10.0f);
    playback.play();
    playback.update(0.015f);
    ASSERT_EQ(data(), v({ 1, 3, 2 }));

    playback.stop();
    EXPECT_TRUE(playback.isStopped());
    EXPECT_EQ(playback.getCurrentOperationIndex(), 0u);
    EXPECT_EQ(data(), v({ 3, 1, 2 }));
}

TEST_F(PlaybackTest, ManualStepsWorkInBothDirections)
{
    EXPECT_FALSE(playback.stepBackward());

    ASSERT_TRUE(playback.stepForward());
    EXPECT_TRUE(playback.isPaused());
    EXPECT_EQ(data(), v({ 1, 3, 2 }));

    ASSERT_TRUE(playback.stepBackward());
    EXPECT_TRUE(playback.isStopped());
    EXPECT_EQ(data(), v({ 3, 1, 2 }));
}

TEST(Playback, OverwriteIsUndoneThroughTheReplacedValue)
{
    auto rec = std::make_shared<Rec>();
    rec->initial = { 4, 5, 6 };
    std::vector<int> work = rec->initial;
    Recorder<int> recorder(rec->statistics);
    rec->operations = { recorder.overwrite(work, 1, 99) };

    Playback<int> playback;
    ASSERT_TRUE(playback.load(rec).has_value());

    ASSERT_TRUE(playback.stepForward());
    EXPECT_EQ(playback.getData()[1], 99);
    ASSERT_TRUE(playback.stepBackward());
    EXPECT_EQ(playback.getData()[1], 5);
}

TEST(Playback, LoadSharesTheRecordingInsteadOfCopyingIt)
{
    const auto rec = swapRecording();
    Playback<int> first, second;
    ASSERT_TRUE(first.load(rec).has_value());
    ASSERT_TRUE(second.load(rec).has_value());

    EXPECT_EQ(first.getOperations().data(), second.getOperations().data());
}

TEST(Playback, LoadTakesOwnershipOfARecordingPassedByValue)
{
    Playback<int> playback;
    ASSERT_TRUE(playback.load(record<int>(AlgorithmType::BubbleSort, std::vector<int>{ 3, 1, 2 })).has_value());

    EXPECT_TRUE(playback.isLoaded());
    while (playback.stepForward()) {}
    EXPECT_EQ(std::vector<int>(playback.getData().begin(), playback.getData().end()), (std::vector<int>{ 1, 2, 3 }));
}

TEST(Playback, LoadByValueRejectsAnEmptyRecording)
{
    Playback<int> playback;
    EXPECT_EQ(playback.load(Rec{}).error(), PlaybackError::EmptyRecording);
    EXPECT_FALSE(playback.isLoaded());
}

TEST(Playback, RecordSharedFeedsSeveralPlaybacksWithoutCopying)
{
    const auto shared = recordShared<int>(AlgorithmType::QuickSort, std::vector<int>{ 4, 2, 5, 1, 3 });
    ASSERT_NE(shared, nullptr);

    Playback<int> bars, heatmap;
    ASSERT_TRUE(bars.load(shared).has_value());
    ASSERT_TRUE(heatmap.load(shared).has_value());
    EXPECT_EQ(bars.getOperations().data(), heatmap.getOperations().data());

    while (bars.stepForward()) {}
    EXPECT_TRUE(bars.isFinished());
    EXPECT_TRUE(heatmap.isStopped()); // each playback keeps its own position
}

TEST(Playback, RecordSharedAcceptsACustomComparator)
{
    const auto shared = recordShared<int>(AlgorithmType::BubbleSort, std::vector<int>{ 1, 3, 2 }, std::greater<int>{});
    Playback<int> playback;
    ASSERT_TRUE(playback.load(shared).has_value());
    while (playback.stepForward()) {}
    EXPECT_EQ(std::vector<int>(playback.getData().begin(), playback.getData().end()), (std::vector<int>{ 3, 2, 1 }));
}

// ------------------------------------------------------------------
// Marks
// ------------------------------------------------------------------

namespace {

// {3, 1, 2}: mark(Pivot, 2); mark(Custom 7, 2); swap(1,2); clear(Pivot); sorted
std::shared_ptr<const Rec> markRecording()
{
    auto rec = std::make_shared<Rec>();
    rec->initial = { 3, 1, 2 };
    std::vector<int> work = rec->initial;
    Recorder<int> recorder(rec->statistics);
    rec->operations = {
        recorder.mark(work, MarkRole::Pivot, 2),
        recorder.mark(work, MarkId::custom_id(7), 2),
        recorder.swap(work, 1, 2),
        recorder.clearMark(MarkRole::Pivot),
        recorder.sorted(),
    };
    return rec;
}

} // namespace

TEST(PlaybackMarks, AMarkStartsWhenItsOperationRunsAndLastsUntilCleared)
{
    Playback<int> playback;
    ASSERT_TRUE(playback.load(markRecording()).has_value());

    EXPECT_EQ(playback.getMarkIndex(MarkRole::Pivot), -1);
    ASSERT_TRUE(playback.stepForward());
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Pivot), 2);

    ASSERT_TRUE(playback.stepForward()); // custom mark: same slot, independent id
    ASSERT_TRUE(playback.stepForward()); // swap
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Pivot), 1);                // follows the element
    EXPECT_EQ(playback.getMarkIndex(MarkId::custom_id(7)), 1);
    EXPECT_EQ(playback.getMarkIndex(MarkId::custom_id(8)), -1);

    ASSERT_TRUE(playback.stepForward()); // clear
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Pivot), -1);
    EXPECT_EQ(playback.getMarkIndex(MarkId::custom_id(7)), 1);
}

TEST(PlaybackMarks, SteppingBackwardRestoresEveryEarlierMark)
{
    Playback<int> playback;
    ASSERT_TRUE(playback.load(markRecording()).has_value());
    while (playback.stepForward()) {}

    ASSERT_TRUE(playback.stepBackward()); // sorted
    ASSERT_TRUE(playback.stepBackward()); // clear: Pivot is back, on the moved element
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Pivot), 1);

    ASSERT_TRUE(playback.stepBackward()); // swap: the marks move back with it
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Pivot), 2);
    EXPECT_EQ(playback.getMarkIndex(MarkId::custom_id(7)), 2);

    ASSERT_TRUE(playback.stepBackward());
    ASSERT_TRUE(playback.stepBackward());
    EXPECT_TRUE(playback.getMarks().empty());
}

TEST(PlaybackMarks, SettingAMarkAgainMovesItAndUndoReturnsIt)
{
    auto rec = std::make_shared<Rec>();
    rec->initial = { 3, 1, 2 };
    Recorder<int> recorder(rec->statistics);
    rec->operations = { recorder.mark(rec->initial, MarkRole::Minimum, 0), recorder.mark(rec->initial, MarkRole::Minimum, 1) };

    Playback<int> playback;
    ASSERT_TRUE(playback.load(rec).has_value());
    ASSERT_TRUE(playback.stepForward());
    ASSERT_TRUE(playback.stepForward());
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Minimum), 1);
    EXPECT_EQ(playback.getMarks().size(), 1u);

    ASSERT_TRUE(playback.stepBackward());
    EXPECT_EQ(playback.getMarkIndex(MarkRole::Minimum), 0);
}

TEST(PlaybackMarks, StopAndClearDropEveryMark)
{
    Playback<int> playback;
    ASSERT_TRUE(playback.load(markRecording()).has_value());
    ASSERT_TRUE(playback.stepForward());

    playback.stop();
    EXPECT_TRUE(playback.getMarks().empty());

    ASSERT_TRUE(playback.stepForward());
    playback.clear();
    EXPECT_TRUE(playback.getMarks().empty());
    EXPECT_FALSE(playback.isLoaded());
}
