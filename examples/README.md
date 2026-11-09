# Sortrace Examples

These programs show how to record sorting runs, replay them, inspect their operations, and present the same data in different ways. They are terminal programs built with Sortrace and the C++ standard library. The rendering and terminal helpers are example code, not part of the library.

- [Sortrace Examples](#sortrace-examples)
  - [Folder layout](#folder-layout)
  - [Interactive CLI](#interactive-cli)
  - [Basic examples](#basic-examples)
  - [Shared helpers](#shared-helpers)
  - [Run the examples](#run-the-examples)

## Folder layout

| Folder               | Contents                                                                 |
| :------------------- | :----------------------------------------------------------------------- |
| [`basic/`](basic/)   | Small standalone programs, each focused on one Sortrace feature.         |
| [`cli/`](cli/)       | The interactive `sortrace_cli` application.                              |
| [`common/`](common/) | Terminal rendering, formatting and input helpers shared by the examples. |

## Interactive CLI

`sortrace_cli` brings the main workflows together. It can record an algorithm in one call or through a time-budgeted `Session`, animate and step through a recording, display its operations, and compare algorithms.

Type `tour` for a guided walkthrough or `help` to see every command. It also accepts startup options for the input size, algorithm, seed and terminal rendering mode.

## Basic examples

Each program in [`basic/`](basic/) focuses on one task. If you are new to the library, read them in this order:

1. [`record_minimal.cpp`](basic/record_minimal.cpp) runs an algorithm to completion with `record()` and prints its statistics.
2. [`playback_animate.cpp`](basic/playback_animate.cpp) animates playback by calling `update(dt)`.
3. [`step_backward.cpp`](basic/step_backward.cpp) steps through a recording in both directions and checks that reversing restores the original data.
4. [`marks.cpp`](basic/marks.cpp) shows how named marks follow elements through swaps.
5. [`session_budget.cpp`](basic/session_budget.cpp) computes a run in small time slices using `Session`.
6. [`compare_algorithms.cpp`](basic/compare_algorithms.cpp) compares all built-in algorithms across ascending, descending, nearly sorted, few-unique and shuffled inputs.
7. [`shared_views.cpp`](basic/shared_views.cpp) uses one shared recording to draw bars and a heatmap in sync.
8. [`export_json.cpp`](basic/export_json.cpp) exports recording data as JSON.

## Shared helpers

The files in [`common/`](common/) are the presentation layer of the terminal examples:

- `render.hpp` draws playback as bars.
- `heatmap.hpp` draws playback as a heatmap.
- `format.hpp` formats operation descriptions, marks and statistics.
- `terminal.hpp` handles terminal capabilities, colors and platform-specific console setup.
- `input.hpp` contains small input-parsing helpers.

They show one way to use the data a `Playback` exposes (see [Architecture](../docs/ARCHITECTURE.md#recording-and-playback)). An application can replace them with its own rendering or output code.

## Run the examples

First [build the tests and examples](../docs/INSTALL.md#build-the-tests-and-examples). The executables are placed under `build/bin/`:

```bash
./build/bin/sortrace_cli
./build/bin/basic/shared_views
```

With a multi-config generator such as Visual Studio, they are under `build/bin/<Config>/` instead, for example `build/bin/Release/basic/shared_views.exe`.

Return to the [project README](../README.md).
