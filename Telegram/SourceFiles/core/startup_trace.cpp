/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/startup_trace.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/resource.h>

namespace Core {
namespace {

[[nodiscard]] long long NowUs() {
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
}

[[nodiscard]] long long CpuUs() {
	rusage usage;
	if (getrusage(RUSAGE_SELF, &usage) != 0) {
		return -1;
	}
	return (long long)(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec)
		* 1000000
		+ usage.ru_utime.tv_usec
		+ usage.ru_stime.tv_usec;
}

} // namespace

void StartupTrace(const char *point) {
	static const auto enabled = (getenv("TDESKTOP_STARTUP_TRACE") != nullptr);
	if (!enabled) {
		return;
	}
	static const auto start = NowUs();
	static auto done = false;
	if (done) {
		return;
	}
	const auto now = NowUs();
	fprintf(
		stderr,
		"startup: %7.1f ms  cpu %7.1f ms  %s\n",
		(now - start) / 1000.,
		CpuUs() / 1000.,
		point);
	fflush(stderr);
	if (!strcmp(point, "first frame")) {
		done = true;
	}
}

} // namespace Core
