# Extending Sortrace

There are two ways to build on Sortrace: add a sorting algorithm, or render a playback in your application. This guide covers both. How recordings work is in [Architecture](ARCHITECTURE.md), and the optional annotations algorithms can emit are in [Marks](MARKS.md).

- [Extending Sortrace](#extending-sortrace)
  - [Add a sorting algorithm](#add-a-sorting-algorithm)
    - [1. Write the algorithm](#1-write-the-algorithm)
    - [2. Register it](#2-register-it)
    - [3. Create it](#3-create-it)
  - [Rules for algorithms](#rules-for-algorithms)
    - [Available steps](#available-steps)
  - [Test an algorithm](#test-an-algorithm)
  - [Troubleshooting](#troubleshooting)
  - [Create a visualization](#create-a-visualization)

## Add a sorting algorithm

The example adds **Gnome Sort** to the built-in algorithm list. It takes three steps.

### 1. Write the algorithm

`include/sortrace/algorithms/builtin/gnome_sort.hpp`

```cpp
#pragma once

#include "sortrace/algorithms/generator_sort.hpp"
#include "sortrace/algorithms/step.hpp"

namespace sortrace {

template <SortableValue T>
class GnomeSort final : public GeneratorSort<T>
{
public:
    explicit GnomeSort(Less<T> less)
        : GeneratorSort<T>(find(AlgorithmType::GnomeSort)->meta, std::move(less)) {}

protected:
    StepGenerator<T> run() override
    {
        using S = Step<T>;
        auto& d = this->m_Data;

        std::size_t i = 1;
        while (i < d.size())
        {
            co_yield S::compare(i - 1, i);
            if (!this->less(d[i], d[i - 1]))
            {
                ++i;
            }
            else
            {
                co_yield S::swap(i - 1, i);
                i = (i > 1) ? i - 1 : i + 1;
            }
        }
    }
};

} // namespace sortrace
```

In a class template, refer to base-class members with `this->`. Use `this->less(a, b)` instead of `<` so the algorithm respects the comparator supplied by the caller.

### 2. Register it

In `include/sortrace/algorithms/registry.hpp`, add the enumerator **before `Count`** and a matching entry in `kAlgorithms`, in the same order:

```cpp
enum class AlgorithmType
{
    ...
    ShellSort,
    GnomeSort,   // new

    Count        // must always be last
};
```

```cpp
{ AlgorithmType::GnomeSort, { "Gnome Sort", "O(n²)", "Moves forward while ordered, steps back after every swap." } },
```

Use a one-sentence description and give the **average-case** complexity. A compile-time check verifies that the registry entries match the enum order.

### 3. Create it

In `include/sortrace/algorithms/factory.hpp`, include the new header and add a `case`:

```cpp
#include "sortrace/algorithms/builtin/gnome_sort.hpp"
...
case AlgorithmType::GnomeSort: return std::make_unique<GnomeSort<T>>(std::move(less));
```

The algorithm is now available through `getAvailableAlgorithms()` and `parseAlgorithm("gnome")`, and the shared test suite covers it automatically.

## Rules for algorithms

1. **Change `m_Data` only through yielded steps.** Reading is fine; writing directly bypasses recording and makes replay incorrect.
2. **Yield a `compare` before each comparison that affects a decision.** Otherwise the [statistics](ARCHITECTURE.md#statistics) undercount comparisons.
3. **A yielded step is applied before the coroutine resumes.** After a `co_yield` returns, `m_Data` reflects that step.
4. **Do not emit `Sorted`.** `GeneratorSort` emits it exactly once when the algorithm finishes, including for empty input.
5. **Handle empty and single-element inputs.** In particular, avoid unsigned underflow from expressions such as `n - 1`.
6. **Keep step indices in range.** `GeneratorSort` validates them; an invalid index throws `std::out_of_range` and the session reports `ComputeError::AlgorithmThrew`.
7. **Do not recurse across coroutine yields.** Use an explicit stack instead; see `quick_sort.hpp`.

### Available steps

| Step                  | Changes `m_Data`? | Effect                                                                                      |
| :-------------------- | :---------------: | :------------------------------------------------------------------------------------------ |
| `S::compare(i, j)`    |        No         | Compares values in `m_Data`.                                                                |
| `S::compareAux(i, j)` |        No         | Compares values in `m_Aux`, the auxiliary buffer.                                           |
| `S::swap(i, j)`       |        Yes        | Exchanges two values in `m_Data`.                                                           |
| `S::overwrite(i, v)`  |        Yes        | Writes a value to `m_Data`; the previous value is recorded for undo.                        |
| `S::mark(id, i)`      |        No         | Sets or moves a mark to an element. See [Marks](MARKS.md#emitting-marks-from-an-algorithm). |
| `S::clearMark(id)`    |        No         | Removes a mark.                                                                             |

Merge-style algorithms often need to write values back from scratch space. `m_Aux` starts as a copy of `m_Data`; you can update it with ordinary C++, then record writes to `m_Data` with `overwrite`. Changes to `m_Aux` are not recorded as playback steps.

## Test an algorithm

`tests/algorithms_test.cpp` runs every registered algorithm on empty, single-element, sorted, reversed, duplicate and mixed inputs. It checks that each algorithm:

- sorts and keeps the same elements, with the default order and with `std::greater`, on `int` and `std::string`,
- ends with exactly one `Sorted` operation,
- reports statistics that match its operations,
- replays forward and backward to the original data,
- keeps marks attached to their elements and clears them,
- reports a slot as settled only when it holds its final value.

A newly registered algorithm is covered automatically. Add a dedicated test only for behavior specific to yours, such as an expected comparison count for a known input.

## Troubleshooting

<details>
<summary>Common symptoms and likely causes</summary>

| Symptom                                        | Likely cause                                                                           |
| :--------------------------------------------- | :------------------------------------------------------------------------------------- |
| `kAlgorithms must list every AlgorithmType...` | The enum and registry entries are missing or out of order.                             |
| A `-Wswitch` warning in `factory.hpp`          | The factory case for the new algorithm is missing.                                     |
| `use of undeclared identifier 'm_Data'`        | A base-class member needs the `this->` prefix.                                         |
| The result is not sorted                       | `m_Data` was modified directly instead of through a yielded step.                      |
| Backward replay does not restore the input     | A data change was not recorded as a step.                                              |
| `AlgorithmThrew`                               | The algorithm threw; check invalid indices and other exceptions in its implementation. |
| Statistics are lower than expected             | A comparison that affects a decision did not yield `compare` or `compareAux`.          |
| A mark stays active                            | The algorithm did not yield `S::clearMark` when the mark stopped applying.             |
| A hang or crash on empty input                 | An empty-input edge case, often `n - 1` underflow, was not handled.                    |

</details>

## Create a visualization

You do not need to extend the library to visualize a run. Include `sortrace/replay/playback.hpp` and read a `Playback`; your application decides how to present the data and operations. The inputs a renderer has are listed in [Architecture](ARCHITECTURE.md#recording-and-playback).

```cpp
void drawHeatmap(const sortrace::Playback<double>& playback, Colormap map)
{
    const auto data = playback.getData();
    for (std::size_t i = 0; i < data.size(); ++i)
        paintCell(i % width, i / width, map(data[i]));

    for (const auto& [id, index] : playback.getMarks())
        outlineCell(index % width, index / width, colorFor(id.role));

    if (const auto* op = playback.getCurrentOperation(); op && op->type == sortrace::OperationType::Swap)
        flashCell(op->indexA), flashCell(op->indexB);
}
```

- Use `isSettled(slot)` for a "finished" style; it is exact during forward and backward playback.
- Use `getCurrentOperation()` to animate only what is about to change, instead of comparing whole arrays.
- Keep animation state (such as easing and positions) in your app; the library only knows the recorded data.
- For custom value types, use `Recording<MyStruct>` with a `Less<MyStruct>` comparator, then map values to whatever suits your renderer.

Runnable renderers live in the [examples](../examples/README.md): bars, a heatmap, and both drawn from one shared recording.
