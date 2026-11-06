<p align="center">
  <img src="docs/brand/sortrace-banner.svg" alt="sortrace banner" width="720" />
</p>

<div align="center">

[![C++23](https://img.shields.io/badge/Language-C%2B%2B23-00599C?style=flat&logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/Build-CMake-064F8C?style=flat&logo=cmake&logoColor=white)](https://cmake.org/)
![Header only](https://img.shields.io/badge/Header--only-yes-4fdcbf?style=flat)
![Status](https://img.shields.io/badge/Status-Beta-blue?style=flat)
![Version](https://img.shields.io/badge/Version-0.1.0-brightgreen?style=flat)
[![License MIT](https://img.shields.io/badge/License-MIT-lightgrey?style=flat)](LICENSE)

</div>

[Quick Start](#quick-start)
• [Examples](#examples)
• [Installation](docs/INSTALL.md)
• [Architecture](docs/ARCHITECTURE.md)
• [Extending](docs/EXTENDING.md)
• [Marks](docs/MARKS.md)

---

**sortrace** is a header-only C++23 library that runs a sorting algorithm **once** and writes down everything it did: every comparison, swap, write and mark. The result is a plain `Recording` you can replay forward and backward, at any speed, in any form.

It deliberately stops there. **How the data is shown is up to you**: bars, a heatmap, a ring of dots, a terminal table, a sound for every swap or a JSON export. The library never knows about colors, windows or pixels, and it has no dependencies.

Feedback and contributions are welcome. ❤️

> [!NOTE]
> This project is in **Beta** (`0.1.0`). The API may still change while it settles.

<div align="center">
  <img src="docs/images/views.svg" alt="The same recording drawn as bars, a heatmap and a circle" width="90%">
</div>

> [!TIP]
> Bars, heatmap and circle are only examples of what a renderer can do with one recording. They are not built-in modes.

## Highlights

| Feature                          | What you get                                                                                              |
| :------------------------------- | :-------------------------------------------------------------------------------------------------------- |
| **Record once, render anywhere** | A `Recording` is plain data: store it, share it, export it. `recordShared()` feeds several views at once. |
| **Fully reversible**             | Every operation keeps the values it touches, so playback runs backward without re-simulating.             |
| **Tracked marks**                | Named marks follow elements through swaps, forward and backward. See [Marks](docs/MARKS.md).              |
| **Consistent statistics**        | Comparisons, swaps, reads, writes and compute time, counted the same way for every algorithm.             |
| **Cooperative execution**        | A `Session` works within a time budget and resumes where it stopped, so a UI never stalls.                |
| **Generic by design**            | Works with `int`, `std::string` or your own types. Errors are `std::expected`; logging is a callback.     |

## Quick Start

A sortrace application has two phases: **record** a run, then **replay** it. Recording never draws anything; replaying never sorts anything.

```cpp
#include <sortrace/sortrace.hpp>

using namespace sortrace;

// 1. Record: run an algorithm to completion over your data.
std::vector<int> data = { 5, 2, 9, 1, 7 };
auto recording = record<int>(AlgorithmType::QuickSort, data);

// 2. Replay: step through it in any direction, whenever you want.
Playback<int> playback;
if (playback.load(std::move(recording))) {
    while (playback.stepForward()) {
        draw(playback.getData(), playback.getMarks(), playback.getCurrentOperation());
    }
}
```

Inside a UI, compute in time slices so the frame never stalls:

```cpp
Session<int> session;
session.setAlgorithm(make<int>(AlgorithmType::MergeSort));
session.setData(data);

// Once per frame, until it completes or reports an error.
auto progress = session.compute(std::chrono::microseconds{ 2500 });
if (!progress)
    onComputeError(progress.error());
else if (*progress == Progress::Complete)
{
    auto loaded = playback.load(session.getRecording());
    if (!loaded)
        onPlaybackError(loaded.error());
    else
        playback.play();
}
```

`onComputeError` and `onPlaybackError` stand for your application's error reporting.

A `Playback` exposes what a renderer needs:

| Call                    | Returns                                        |
| :---------------------- | :--------------------------------------------- |
| `getData()`             | The data after every operation applied so far. |
| `getCurrentOperation()` | The operation the next step will apply.        |
| `getMarks()`            | Active marks and the elements they point at.   |
| `isSettled(slot)`       | Whether a slot already holds its final value.  |
| `getProgress()`         | How far the playback is, from 0 to 1.          |

For the full pipeline and guarantees, see [Architecture](docs/ARCHITECTURE.md).

## Examples

Everything runs in the terminal and uses only the library and the standard library. The shared drawing code lives in [`examples/common/`](examples/common), so the small examples and the full CLI draw the same bars.

**`sortrace_cli`** is an interactive terminal program that puts the whole library to work: record an algorithm, watch it replay, step through it with the arrow keys, run a `Session`, and compare algorithms. Type `tour` for a guided walk-through or `help` for every command.

```bash
./build/bin/sortrace_cli
```

Its source is in [`examples/cli/`](examples/cli).

---

**Small examples** in [`examples/basic/`](examples/basic), one idea each:

| Example                                                     | What it shows                                                                         |
| :---------------------------------------------------------- | :------------------------------------------------------------------------------------ |
| [record_minimal](examples/basic/record_minimal.cpp)         | `record()`: run to the end in one call and read the statistics                        |
| [compare_algorithms](examples/basic/compare_algorithms.cpp) | Every algorithm against sorted, reversed, nearly sorted, few-unique and shuffled data |
| [session_budget](examples/basic/session_budget.cpp)         | `Session`: compute in small pieces, one per frame                                     |
| [playback_animate](examples/basic/playback_animate.cpp)     | Replay by time with `update(dt)`                                                      |
| [step_backward](examples/basic/step_backward.cpp)           | Replay by hand, forward and backward                                                  |
| [marks](examples/basic/marks.cpp)                           | Marks that follow their element through swaps                                         |
| [shared_views](examples/basic/shared_views.cpp)             | One `recordShared()` run drawn as bars and as a heatmap                               |
| [export_json](examples/basic/export_json.cpp)               | A `Recording` is plain data: dump it as JSON                                          |

Build them as described in [the installation guide](docs/INSTALL.md#build-the-tests-and-examples); the small ones land in a `basic/` folder next to `sortrace_cli`.

## Built-in algorithms

| Algorithm                       | Average     | Marks     |
| :------------------------------ | :---------- | :-------- |
| Bubble, Cocktail, Insertion     | O(n²)       |           |
| Selection                       | O(n²)       | `Minimum` |
| Quick (Lomuto, median of three) | O(n log n)  | `Pivot`   |
| Merge (bottom-up)               | O(n log n)  |           |
| Heap                            | O(n log n)  |           |
| Shell                           | O(n log² n) |           |

To add your own, see [Extending sortrace](docs/EXTENDING.md).

## Installation

sortrace is header-only: add it to your CMake project and link it.

```cmake
add_subdirectory(third_party/sortrace)
target_link_libraries(my_app PRIVATE sortrace::sortrace)
```

Requirements, `find_package`, building the tests and examples, and the CMake options are in [the installation guide](docs/INSTALL.md).

## Contributing

Contributions are always welcome! ❤️ Whether you are reporting bugs, fixing issues, adding algorithms or improving the documentation, your help is appreciated.

Before opening a pull request:

- Keep pull requests focused: prefer small, atomic PRs that address a single feature or fix.
- Add or update tests; every registered algorithm is covered by the shared suite automatically.
- Ensure the project builds cleanly without new warnings (`-DSORTRACE_STRICT_WARNINGS=ON`).

For major changes or new features, please open an issue first to discuss what you would like to change.

## License

This project is licensed under the **MIT License**. <br>
See the [LICENSE](LICENSE) file for details.
