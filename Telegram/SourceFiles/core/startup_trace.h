/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Core {

// With TDESKTOP_STARTUP_TRACE in the environment, prints to stderr the
// wall time since main() and the CPU time of the process at each named
// point of the start (the first point also shows the CPU time spent before
// main(): loading and relocating the program and its libraries).
void StartupTrace(const char *point);

} // namespace Core
