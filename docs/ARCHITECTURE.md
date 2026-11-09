# Architecture

Sortrace separates **computing** a sort from **showing** it. This page covers the pipeline, the two ways of running an algorithm, how statistics are counted, and the guarantees a recording gives its renderer.

- [Architecture](#architecture)
  - [The pipeline](#the-pipeline)
  - [Operations](#operations)
  - [Marks](#marks)
  - [Recording and Playback](#recording-and-playback)
  - [Execution](#execution)
  - [Statistics](#statistics)
  - [Guarantees](#guarantees)
  - [Errors and logging](#errors-and-logging)

## The pipeline

<div align="center">
  <img src="images/pipeline.svg" alt="Algorithm, GeneratorSort, Recorder, Recording and Playback in sequence" width="95%">
</div>

1. An **algorithm** is a coroutine that only _describes_ what it wants: `co_yield Step::swap(i, j)`.
2. **GeneratorSort** validates the step and applies it through the **Recorder**.
3. The **Recorder** edits the working buffer, updates the statistics and returns an `Operation`.
4. The operations, the initial data and the statistics form a **Recording**.
5. A **Playback** replays it over its own copy of the data, forward and backward.

The algorithm never runs while you watch. That is what makes stepping backward, scrubbing and comparing several algorithms on identical data cheap.

## Operations

<div align="center">
  <img src="images/operations.svg" alt="Compare, Swap, Overwrite, MarkSet, MarkClear and Sorted drawn on an array" width="85%">
</div>

An operation records one event in the run:

- **`Compare`** reads two slots (`indexA`, `indexB`) and stores both values. It does not change the data.
- **`Swap`** exchanges two slots and stores their values from before the exchange.
- **`Overwrite`** writes a value to `indexA`; `valueA` is the new value and `valueB` is the value it replaced.
- **`MarkSet`** points a mark at `indexA` and stores the value there. **`MarkClear`** removes that mark.
- **`Sorted`** signals the end of the run.

Fields that do not apply to an operation use `-1` for indices and a default value for values. Mark operations also carry a `MarkId`. Because operations retain the values they touch, playback can undo each one in O(1).

## Marks

Marks optionally identify elements of interest, such as pivots or current minima. Playback tracks them through swaps in either direction; an overwrite leaves a mark at its existing index. Renderers may ignore marks.

See [Marks](MARKS.md) for roles, identity, the lifecycle and the full behavior reference.

## Recording and Playback

A `Recording<T>` holds the algorithm name, the original input, the ordered operations, and the run's statistics. `record()` returns a recording by value, which the caller owns. For sharing, `recordShared()` and a completed `Session` provide a `std::shared_ptr<const Recording<T>>`; the `const` handle prevents changes to the recorded run.

`Playback::load()` accepts either that shared pointer or a recording by value, for example `playback.load(record<int>(type, data))`. Each playback keeps its own position, visible data and marks, so several playbacks can share one recording while advancing independently:

```cpp
auto run = recordShared<int>(AlgorithmType::QuickSort, data);

Playback<int> bars, heatmap;
const auto barsLoaded = bars.load(run);
const auto heatmapLoaded = heatmap.load(run);
if (!barsLoaded || !heatmapLoaded)
    return; // Handle the PlaybackError in your application.
```

A runnable version is [shared_views](../examples/basic/shared_views.cpp).

`Playback<T>` exposes everything a renderer needs:

| Need          | Call                                          |
| :------------ | :-------------------------------------------- |
| Current data  | `getData()`                                   |
| Current event | `getCurrentOperation()`                       |
| Marks         | `getMarks()`, `getMarkIndex(id)`              |
| Settled slots | `isSettled(slot)`                             |
| Progress      | `getProgress()`, `getCurrentOperationIndex()` |

Time is optional. A CLI calls `stepForward()` in a loop. A UI calls `update(deltaSeconds)` each frame, with `setDelayMs()` for the speed and `enableDelay(false)` to finish instantly.

A playback is always `Stopped` (first operation, data untouched), `Playing`, `Paused` or `Finished`. `play()` starts from Stopped, resumes from Paused and restarts from Finished.

## Execution

Both `record()` and `Session<T>` execute an algorithm and produce a `Recording<T>`. Choose based on whether running the algorithm should block the caller.

**Use `record()`** for CLI tools, tests, or any case where running to completion in one call is fine.

**Use `Session<T>`** when execution must stay responsive, such as in a UI or when comparing runs. It computes in time slices and resumes between calls:

```cpp
auto progress = session.compute(std::chrono::microseconds{ 2500 }); // call again while InProgress
```

See [session_budget](../examples/basic/session_budget.cpp) for the full loop.

- A budget limits work approximately: elapsed time is checked every 256 algorithm steps, so a call can overshoot. A zero budget runs to completion and is refused above `SessionOptions::blockingLimit`, because O(n²) algorithms can produce millions of operations.
- Each session keeps its own run state, so many sessions can run side by side (comparison views), and `getSessionId()` gives each one a stable, unique identity.
- When complete, `getRecording()` hands out the shared recording. A `Session` knows nothing about playback.

## Statistics

The Recorder counts as a byproduct of building each operation, so every algorithm is measured the same way:

| Step                     | Comparisons | Swaps | Reads | Writes | Operations |
| :----------------------- | :---------: | :---: | :---: | :----: | :--------: |
| `compare` / `compareAux` |     +1      |   —   |  +2   |   —    |     +1     |
| `swap`                   |      —      |  +1   |  +2   |   +2   |     +1     |
| `overwrite`              |      —      |   —   |  +1   |   +1   |     +1     |
| `mark` / `clearMark`     |      —      |   —   |   —   |   —    |     +1     |

Ordinary reads, writes to the auxiliary buffer and local computation are not counted, and early exits (such as stopping after a pass without swaps) change the totals. Compute time covers only the algorithm's stepping, never playback. Statistics are available while a `Session` is still running and are final once its recording is complete.

## Guarantees

- **Replay is exact.** Playing a recording forward reproduces the algorithm's result; stepping all the way back restores the input.
- **Statistics cannot lie.** The Recorder builds each operation from the real buffer and counts as a byproduct. Algorithms cannot hand-write operations.
- **One `Sorted`, always last**, also for empty and one-element input.
- **Indices are validated** at the boundary: when an algorithm yields a step, and again when a recording is loaded. Replaying needs no per-step checks.
- **Settled means final.** `isSettled(slot)` is true only once no remaining operation changes that slot.
- **Recordings are values.** Nothing in a recording points into a session or an algorithm.

## Errors and logging

Expected failures are returned as values rather than thrown as exceptions:

- `Session::compute()` returns `std::expected<Progress, ComputeError>`.
- `Playback::load()` returns `std::expected<void, PlaybackError>`.

`ComputeError` can be `NoAlgorithm`, `NoData`, `TooLargeForBlocking` or `AlgorithmThrew`. `PlaybackError` can be `EmptyRecording` or `InvalidOperation`.

The library is **silent by default** and never asserts on missing setup. To see its diagnostics, give any `Session` or `Playback` a sink:

```cpp
session.setLogSink([](sortrace::LogLevel level, std::string_view message) {
    my_logger.write(level, message); // spdlog, stderr... your choice.
});
```

Messages are formatted only when a sink is set.

---

Next: [Extending](EXTENDING.md) to add an algorithm or a renderer, or back to the [README](../README.md).
