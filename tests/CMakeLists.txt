# =============================================================================
#  Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
#  SPDX-License-Identifier: MIT
# =============================================================================

# ------------------------------------------------------------------------------
#  Dependency: GoogleTest
# ------------------------------------------------------------------------------
include(FetchContent)

find_package(GTest QUIET)

if(NOT GTest_FOUND)
  # Match MSVC runtime settings to avoid mismatches on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT for gtest" FORCE)

  # Disable building the gmock framework to reduce build footprint.
  set(BUILD_GMOCK OFF CACHE BOOL "Disable gmock framework" FORCE)

  # Disable installation rules for GoogleTest to avoid polluting system paths.
  set(INSTALL_GTEST OFF CACHE BOOL "Disable gtest installation rules" FORCE)

  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
    GIT_SHALLOW    TRUE
    SYSTEM
  )
  FetchContent_MakeAvailable(googletest)
endif()

# Load the native GoogleTest discovery modules
include(GoogleTest)

# ------------------------------------------------------------------------------
#  Source File Maps
# ------------------------------------------------------------------------------
file(GLOB_RECURSE SORTRACE_GTEST_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(NOT SORTRACE_GTEST_SOURCES)
  message(WARNING "No sortrace unit test sources were found.")
  return()
endif()

# ------------------------------------------------------------------------------
#  Test Binary Target Initialization
# ------------------------------------------------------------------------------

add_executable(sortrace_tests ${SORTRACE_GTEST_SOURCES})

target_link_libraries(sortrace_tests
  PRIVATE
    sortrace::sortrace
    GTest::gtest
    GTest::gtest_main
)
sortrace_enable_warnings(sortrace_tests)

# ------------------------------------------------------------------------------
#  Automated Test Discovery Registry
# ------------------------------------------------------------------------------
gtest_discover_tests(sortrace_tests DISCOVERY_MODE PRE_TEST)
