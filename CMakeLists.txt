# =============================================================================
#  Header-only C++23 library for replayable sorting algorithm traces.
# -----------------------------------------------------------------------------
#  Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
#  SPDX-License-Identifier: MIT
# =============================================================================

cmake_minimum_required(VERSION 3.23)

project(sortrace
  VERSION     0.1.0
  DESCRIPTION "Records sorting algorithms as replayable operation traces"
  LANGUAGES   CXX
)

option(SORTRACE_BUILD_TESTS     "Build the unit tests"       ${PROJECT_IS_TOP_LEVEL})
option(SORTRACE_BUILD_EXAMPLES  "Build the examples"         ${PROJECT_IS_TOP_LEVEL})
option(SORTRACE_STRICT_WARNINGS "Treat warnings as errors"   OFF)

if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()

# ------------------------------------------------------------------------------
#  Library
# ------------------------------------------------------------------------------
add_library(sortrace INTERFACE)
add_library(sortrace::sortrace ALIAS sortrace)

target_compile_features(sortrace INTERFACE cxx_std_23)
target_include_directories(sortrace INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Warnings for the targets that live in this repository (tests, examples).
function(sortrace_enable_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 $<$<BOOL:${SORTRACE_STRICT_WARNINGS}>:/WX>)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
                           $<$<BOOL:${SORTRACE_STRICT_WARNINGS}>:-Werror>)
  endif()
endfunction()

# ------------------------------------------------------------------------------
#  Install
# ------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(TARGETS sortrace EXPORT sortraceTargets)
  install(
    EXPORT      sortraceTargets
    FILE        sortraceTargets.cmake
    NAMESPACE   sortrace::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortrace
  )

  configure_package_config_file(cmake/sortraceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sortraceConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortrace)
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/sortraceConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sortraceConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/sortraceConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sortrace)
endif()

# ------------------------------------------------------------------------------
#  Examples & tests
# ------------------------------------------------------------------------------
if(SORTRACE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(SORTRACE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
