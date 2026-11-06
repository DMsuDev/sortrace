# Installation

`sortrace` is header-only: there is nothing to compile to use it. A C++23 compiler and CMake are all you need; the rest is for building the tests and examples.

- [Installation](#installation)
  - [Requirements](#requirements)
  - [Use it in your project](#use-it-in-your-project)
  - [Build the tests and examples](#build-the-tests-and-examples)
  - [CMake options](#cmake-options)

## Requirements

| Tool             | Version | Notes                                                                   |
| :--------------- | :------ | :---------------------------------------------------------------------- |
| **CMake**        | 3.23+   | Build system generator                                                  |
| **C++ Compiler** | C++23   | GCC 13+, Clang 17+ · MSVC 19.36+ (needs coroutines and `std::expected`) |
| **GoogleTest**   | Any     | Only for the tests; found on the system or fetched automatically        |

## Use it in your project

As a subdirectory:

```cmake
add_subdirectory(third_party/sortrace)
target_link_libraries(my_app PRIVATE sortrace::sortrace)
```

Or, after `cmake --install`:

```cmake
find_package(sortrace REQUIRED)
target_link_libraries(my_app PRIVATE sortrace::sortrace)
```

## Build the tests and examples

Only needed to try the examples or to contribute.

```bash
git clone https://github.com/DMsuDev/sortrace.git
cd sortrace
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Executables go to `build/bin/`. The small examples are in a `basic/` folder next to `sortrace_cli`, for example `build/bin/basic/marks`, or `build/bin/Release/basic/marks.exe` with multi-config generators such as Visual Studio.

## CMake options

| Option                     | Default           | Description              |
| :------------------------- | :---------------- | :----------------------- |
| `SORTRACE_BUILD_TESTS`     | ON when top-level | Build the unit tests     |
| `SORTRACE_BUILD_EXAMPLES`  | ON when top-level | Build the examples       |
| `SORTRACE_STRICT_WARNINGS` | OFF               | Treat warnings as errors |

```bash
cmake -S . -B build -DSORTRACE_BUILD_TESTS=OFF -DSORTRACE_STRICT_WARNINGS=ON
```

Back to the [README](../README.md).
