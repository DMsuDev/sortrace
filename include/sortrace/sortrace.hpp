// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// sortrace: sorting algorithms recorded as replayable traces.
// Include this header to get the whole public API.

#pragma once

// Core: shared types and contracts.
#include "sortrace/core/concepts.hpp"
#include "sortrace/core/log.hpp"
#include "sortrace/core/mark.hpp"
#include "sortrace/core/operation.hpp"
#include "sortrace/core/recorder.hpp"
#include "sortrace/core/recording.hpp"
#include "sortrace/core/statistics.hpp"

// Algorithms: contracts, steps, catalog and built-in implementations.
#include "sortrace/algorithms/algorithm.hpp"
#include "sortrace/algorithms/factory.hpp"
#include "sortrace/algorithms/generator.hpp"
#include "sortrace/algorithms/generator_sort.hpp"
#include "sortrace/algorithms/registry.hpp"
#include "sortrace/algorithms/step.hpp"

// Execution: run an algorithm and capture what it did.
#include "sortrace/execution/record.hpp"
#include "sortrace/execution/session.hpp"

// Replay: replay a recording forward and backward.
#include "sortrace/replay/playback.hpp"

// Data utilities.
#include "sortrace/data/data.hpp"
#include "sortrace/data/seed.hpp"

// Version.
#include "sortrace/version.hpp"
