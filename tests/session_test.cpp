// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#include <sortrace/sortrace.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>
#include <vector>

using namespace sortrace;
using namespace std::chrono_literals;

namespace {

std::vector<int> shuffled(int count)
{
    std::mt19937 rng{ 42 };
    return DataSetBuilder::buildShuffled<int>({ .count = count, .maxCount = count }, rng);
}

Session<int> makeSession(int count, SessionOptions options = {}, AlgorithmType type = AlgorithmType::BubbleSort)
{
    Session<int> session(options);
    session.setAlgorithm(make<int>(type));
    session.setData(shuffled(count));
    return session;
}

// A custom algorithm: shows that anything implementing SortAlgorithm<T> plugs into a Session.
class ThrowingAlgorithm final : public SortAlgorithm<int>
{
public:
    enum class When { OnLoad, OnStep };
    explicit ThrowingAlgorithm(When when) : m_When(when) {}

    void load(std::span<const int>) override
    {
        if (m_When == When::OnLoad)
            throw std::runtime_error("load failed");
    }
    std::optional<Operation<int>> step(Recorder<int>&) override { throw std::runtime_error("step failed"); }
    std::span<const int> getData() const override { return {}; }
    std::string_view getName() const override { return "Throwing"; }
    std::string_view getComplexity() const override { return "N/A"; }
    std::string_view getDescription() const override { return "Always throws."; }

private:
    When m_When;
};

} // namespace

TEST(Session, ComputeReportsWhyItFailed)
{
    Session<int> empty;
    EXPECT_EQ(empty.compute().error(), ComputeError::NoAlgorithm);

    Session<int> noData;
    noData.setAlgorithm(make<int>(AlgorithmType::BubbleSort));
    EXPECT_EQ(noData.compute().error(), ComputeError::NoData);

    auto tooBig = makeSession(20, { .blockingLimit = 8 });
    EXPECT_EQ(tooBig.compute().error(), ComputeError::TooLargeForBlocking);

    for (const auto when : { ThrowingAlgorithm::When::OnLoad, ThrowingAlgorithm::When::OnStep })
    {
        auto session = makeSession(16);
        session.setAlgorithm(std::make_unique<ThrowingAlgorithm>(when));
        EXPECT_EQ(session.compute().error(), ComputeError::AlgorithmThrew);
        EXPECT_EQ(session.getRecording(), nullptr); // partial results are discarded
        EXPECT_FALSE(session.isComputing());
    }
}

TEST(Session, ASuccessfulRunProducesASharedRecording)
{
    auto session = makeSession(64);
    EXPECT_EQ(session.getRecording(), nullptr);

    ASSERT_EQ(session.compute(), Progress::Complete);
    const auto recording = session.getRecording();
    ASSERT_NE(recording, nullptr);
    EXPECT_EQ(recording->operations.back().type, OperationType::Sorted);
    EXPECT_EQ(recording->algorithm, "Bubble Sort");
    EXPECT_EQ(session.getStatistics().operationCount, recording->operations.size());

    ASSERT_EQ(session.compute(), Progress::Complete); // asking again changes nothing
    EXPECT_EQ(session.getRecording(), recording);
}

TEST(Session, BudgetedComputeProducesTheSameRecordingAsBlocking)
{
    auto blocking = makeSession(128);
    ASSERT_EQ(blocking.compute(), Progress::Complete);

    auto budgeted = makeSession(128);
    std::expected<Progress, ComputeError> status = Progress::InProgress;
    int slices = 0;
    while (status == Progress::InProgress)
    {
        status = budgeted.compute(1us);
        ++slices;
    }
    ASSERT_EQ(status, Progress::Complete);
    EXPECT_GT(slices, 1);

    const auto a = blocking.getRecording();
    const auto b = budgeted.getRecording();
    ASSERT_EQ(a->operations.size(), b->operations.size());
    EXPECT_EQ(a->statistics.swaps, b->statistics.swaps);
    EXPECT_EQ(a->statistics.comparisons, b->statistics.comparisons);
}

TEST(Session, RefusingABlockingResumeLeavesTheCooperativeRunResumable)
{
    auto session = makeSession(300, { .blockingLimit = 64 });

    ASSERT_EQ(session.compute(1us), Progress::InProgress);
    EXPECT_EQ(session.compute().error(), ComputeError::TooLargeForBlocking);
    EXPECT_TRUE(session.isComputing());
    EXPECT_EQ(session.compute(1us), Progress::InProgress);
}

TEST(Session, ChangingTheAlgorithmOrTheDataDiscardsTheResult)
{
    auto session = makeSession(32);
    ASSERT_EQ(session.compute(), Progress::Complete);

    session.setAlgorithm(make<int>(AlgorithmType::QuickSort));
    EXPECT_EQ(session.getRecording(), nullptr);
    EXPECT_EQ(session.getStatistics().operationCount, 0u);

    ASSERT_EQ(session.compute(), Progress::Complete);
    session.setData(shuffled(16));
    EXPECT_EQ(session.getRecording(), nullptr);
}

TEST(Session, ASessionKeepsWorkingAfterAMove)
{
    auto original = makeSession(32);
    const auto id = original.getSessionId();

    Session<int> moved = std::move(original);
    ASSERT_EQ(moved.compute(), Progress::Complete);
    EXPECT_EQ(moved.getSessionId(), id);
}

TEST(Session, IdsAreUniqueEvenAcrossValueTypes)
{
    Session<int> a, b;
    Session<double> c;
    EXPECT_NE(a.getSessionId(), b.getSessionId());
    EXPECT_NE(b.getSessionId(), c.getSessionId());
}

TEST(Session, LogsOnlyWhenASinkIsSet)
{
    auto quiet = makeSession(16);
    EXPECT_EQ(quiet.compute(), Progress::Complete); // no sink: silent, no crash

    int infos = 0;
    auto loud = makeSession(16);
    loud.setLogSink([&](LogLevel level, std::string_view) { infos += (level == LogLevel::Info); });
    ASSERT_EQ(loud.compute(), Progress::Complete);
    EXPECT_EQ(infos, 1);
}
