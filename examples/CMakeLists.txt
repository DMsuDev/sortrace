# =============================================================================
#  Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
#  SPDX-License-Identifier: MIT
# =============================================================================

# ------------------------------------------------------------------------------
#  Shared Helpers (examples/common)
# ------------------------------------------------------------------------------
add_library(sortrace_examples_common INTERFACE)
target_include_directories(sortrace_examples_common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(sortrace_examples_common INTERFACE sortrace::sortrace)

# ------------------------------------------------------------------------------
#  Basic Examples: one executable per file in examples/basic, built into a basic/ folder next to sortrace_cli
# ------------------------------------------------------------------------------
file(GLOB SORTRACE_BASIC_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/basic/*.cpp"
)

foreach(source IN LISTS SORTRACE_BASIC_SOURCES)
  get_filename_component(name "${source}" NAME_WE)

  add_executable(example_${name} "${source}")
  target_link_libraries(example_${name} PRIVATE sortrace_examples_common)
  set_target_properties(example_${name} PROPERTIES
    OUTPUT_NAME              ${name}
    RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:sortrace_cli>/basic"
  )
  sortrace_enable_warnings(example_${name})
endforeach()

# ------------------------------------------------------------------------------
#  Interactive CLI
# ------------------------------------------------------------------------------
add_executable(sortrace_cli cli/main.cpp cli/commands.cpp)
target_link_libraries(sortrace_cli PRIVATE sortrace_examples_common)
sortrace_enable_warnings(sortrace_cli)
