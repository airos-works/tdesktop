// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "base/platform/haiku/base_last_input_haiku.h"

#include "base/platform/base_platform_last_input.h"

#include "base/platform/haiku/base_haiku_begin.h"
#include <InterfaceDefs.h>
#include <OS.h>
#include "base/platform/haiku/base_haiku_end.h"

namespace base::Platform {

// input_server tells the kernel about every key and mouse event:
// idle_time() is the time since the last of them, for the whole desktop.
std::optional<crl::time> LastUserInputTime() {
	const auto idle = idle_time();
	if (idle < 0) {
		return std::nullopt;
	}
	return crl::now() - crl::time(idle / 1000);
}

} // namespace base::Platform
