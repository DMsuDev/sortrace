// Copyright (c) 2026 Dayron Mustelier (@DMsuDev)
// SPDX-License-Identifier: MIT

// The commands that run and replay algorithms.
#pragma once

#include "app.hpp"

namespace sortrace::examples::cli {

void cmdRecord(App& app);                  // MODE 1: record()
void cmdCompare(const App& app);           // record() every algorithm
void cmdSession(App& app, bool blocking);  // MODE 2: Session
void cmdPlay(App& app, double stepsPerSecond); // Playback by time
void cmdStep(App& app);                    // Playback by hand
void cmdTour(const App& current);          // Guided demo, on a copy of the state

} // namespace sortrace::examples::cli
