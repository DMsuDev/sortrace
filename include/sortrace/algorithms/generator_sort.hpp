// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

#pragma once

#include "sortrace/algorithms/algorithm.hpp"
#include "sortrace/algorithms/registry.hpp"
#include "sortrace/algorithms/generator.hpp"

#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sortrace {

/// Base for algorithms written as straight-line code that yields one Step per operation.
///
/// Rules for run():
///  - Read m_Data freely; change it ONLY through yielded Steps (swap / overwrite).
///  - Yield a compare before every decision between two elements, so statistics stay honest.
///  - After a co_yield returns, the step has already been applied.
///  - Do not emit Sorted: step() does it when run() ends.
///  - A bad index throws std::out_of_range (Session reports it as ComputeError::AlgorithmThrew).
template <SortableValue T>
class GeneratorSort : public SortAlgorithm<T>
{
public:
    GeneratorSort(AlgorithmMeta meta, Less<T> less) : m_Less(std::move(less)), m_Meta(meta) {}

    // The coroutine holds `this`: an instance must never move or be copied.
    GeneratorSort(const GeneratorSort&)            = delete;
    GeneratorSort& operator=(const GeneratorSort&) = delete;
    GeneratorSort(GeneratorSort&&)                 = delete;
    GeneratorSort& operator=(GeneratorSort&&)      = delete;

    void load(std::span<const T> data) final
    {
        m_Data.assign(data.begin(), data.end());
        m_Aux.assign(data.begin(), data.end());
        m_Finished  = false;
        m_Generator = run(); // Lazy: nothing executes until the first step().
    }

    std::optional<Operation<T>> step(Recorder<T>& recorder) final
    {
        if (m_Finished)
            return std::nullopt;

        if (!m_Generator.next())
        {
            m_Finished = true;
            return recorder.sorted();
        }

        const Step<T>& s = m_Generator.value();
        validate(s);

        switch (s.kind)
        {
            case Step<T>::Kind::Compare:
                return recorder.compare(s.aux ? std::span<const T>(m_Aux) : std::span<const T>(m_Data), s.a, s.b);
            case Step<T>::Kind::Swap:      return recorder.swap(m_Data, s.a, s.b);
            case Step<T>::Kind::Overwrite: return recorder.overwrite(m_Data, s.a, s.value);
            case Step<T>::Kind::Mark:      return recorder.mark(m_Data, s.markId, s.a);
            case Step<T>::Kind::ClearMark: return recorder.clearMark(s.markId);
        }
        return std::nullopt;
    }

    [[nodiscard]] std::span<const T> getData() const final { return m_Data; }

    [[nodiscard]] std::string_view getName() const final        { return m_Meta.name; }
    [[nodiscard]] std::string_view getComplexity() const final  { return m_Meta.complexity; }
    [[nodiscard]] std::string_view getDescription() const final { return m_Meta.description; }

protected:
    /// The algorithm itself. m_Aux is scratch space, initially a copy of m_Data.
    virtual StepGenerator<T> run() = 0;

    [[nodiscard]] bool less(const T& a, const T& b) const { return m_Less(a, b); }

    std::vector<T> m_Data;
    std::vector<T> m_Aux;

private:
    static void requireInRange(std::size_t index, std::size_t size, const char* which)
    {
        if (index >= size)
            throw std::out_of_range(std::string("GeneratorSort: step index ") + which + " = " + std::to_string(index) +
                                    " is out of range (size " + std::to_string(size) + ")");
    }

    void validate(const Step<T>& s) const
    {
        switch (s.kind)
        {
            case Step<T>::Kind::Compare:
            {
                const std::size_t size = s.aux ? m_Aux.size() : m_Data.size();
                requireInRange(s.a, size, "a");
                requireInRange(s.b, size, "b");
                break;
            }
            case Step<T>::Kind::Swap:
                requireInRange(s.a, m_Data.size(), "a");
                requireInRange(s.b, m_Data.size(), "b");
                break;
            case Step<T>::Kind::Overwrite:
            case Step<T>::Kind::Mark:
                requireInRange(s.a, m_Data.size(), "a");
                break;
            case Step<T>::Kind::ClearMark:
                break;
        }
    }

    Less<T>          m_Less;
    AlgorithmMeta    m_Meta;
    StepGenerator<T> m_Generator;
    bool             m_Finished = false;
};

} // namespace sortrace
