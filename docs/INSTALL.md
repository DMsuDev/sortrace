# Installation

Sortrace is header-only: there is nothing to compile to use it. All you need is a C++23 compiler and, for the CMake targets, CMake 3.23 or newer. Everything else on this page is for building the tests and examples.

- [Installation](#installation)
  - [Requirements](#requirements)
  - [Add Sortrace to your project](#add-sortrace-to-your-project)
    - [As a subdirectory](#as-a-subdirectory)
    - [With FetchContent](#with-fetchcontent)
    - [As an installed package](#as-an-installed-package)
    - [Without CMake](#without-cmake)
  - [Build the tests and examples](#build-the-tests-and-examples)
  - [CMake options](#cmake-options)
  - [Install the headers](#install-the-headers)
  - [Troubleshooting](#troubleshooting)

## Requirements

| Tool             | Needed for                                          | Notes                                                             |
| :--------------- | :-------------------------------------------------- | :---------------------------------------------------------------- |
| **C++ compiler** | Using the library                                   | C++23 with coroutines, `std::expected` and `<format>`.            |
| **CMake**        | The `sortrace::sortrace` target, tests and examples | Version 3.23 or newer. Not required if you only copy the headers. |
| **GoogleTest**   | Tests only                                          | Used from the system if found, otherwise fetched automatically.   |

<details>
<summary>Compiler support</summary>

The library relies on three C++23-era features, so it needs a compiler and standard library that provide them: GCC 13 or newer, Clang 17 or newer, or MSVC 19.36 or newer (Visual Studio 2022 17.6).

</details>

## Add Sortrace to your project

Pick the option that matches how you manage dependencies. All of them end with the same line: link `sortrace::sortrace` and include `<sortrace/sortrace.hpp>`.

### As a subdirectory

Vendor the repository (a git submodule works well) and add it to your build:

```cmake
add_subdirectory(third_party/sortrace)
target_link_libraries(my_app PRIVATE sortrace::sortrace)
```

When Sortrace is not the top-level project, its tests and examples are off by default.

### With FetchContent

Let CMake download it at configure time:

```cmake
include(FetchContent)

FetchContent_Declare(
  sortrace
  GIT_REPOSITORY https://github.com/DMsuDev/sortrace.git
  GIT_TAG        main   # pin a release tag or commit hash for reproducible builds
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(sortrace)

target_link_libraries(my_app PRIVATE sortrace::sortrace)
```

### As an installed package

After [installing the headers](#install-the-headers):

```cmake
find_package(sortrace REQUIRED)
target_link_libraries(my_app PRIVATE sortrace::sortrace)
```

### Without CMake

Copy the `include/sortrace` folder into your project, add its parent directory to your include path, and compile with C++23 enabled (`-std=c++23` or `/std:c++latest`).

## Build the tests and examples

This is only needed to try the examples or to contribute.

```bash
git clone https://github.com/DMsuDev/sortrace.git
cd sortrace
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

With a multi-config generator such as Visual Studio, name the configuration for both steps:

```bash
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

To find and run the programs, see [Run the examples](../examples/README.md#run-the-examples).

## CMake options

| Option                     | Default           | Description               |
| :------------------------- | :---------------- | :------------------------ |
| `SORTRACE_BUILD_TESTS`     | ON when top-level | Build the unit tests.     |
| `SORTRACE_BUILD_EXAMPLES`  | ON when top-level | Build the examples.       |
| `SORTRACE_STRICT_WARNINGS` | OFF               | Treat warnings as errors. |

```bash
cmake -S . -B build -DSORTRACE_BUILD_TESTS=OFF -DSORTRACE_STRICT_WARNINGS=ON
```

## Install the headers

To make Sortrace available to `find_package`, install it to a prefix of your choice:

```bash
cmake -S . -B build
cmake --install build --prefix /path/to/prefix
```

Then point your project at that prefix when configuring it:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/prefix
```

## Troubleshooting

| Symptom                                                 | Likely cause and fix                                                                                |
| :------------------------------------------------------ | :-------------------------------------------------------------------------------------------------- |
| `std::expected` or `<format>` not found                 | The compiler or standard library is too old. Upgrade it, or select a newer toolchain.               |
| Coroutine errors (`<coroutine>`, `co_yield`)            | C++23 is not enabled. Use `-std=c++23` or `/std:c++latest`, or let CMake set it through the target. |
| GoogleTest cannot be downloaded (offline or restricted) | Install GoogleTest on the system, or configure with `-DSORTRACE_BUILD_TESTS=OFF`.                   |

Back to the [README](../README.md).
