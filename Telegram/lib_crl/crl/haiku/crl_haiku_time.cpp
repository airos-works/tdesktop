// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include <crl/crl_time.h>

#ifdef CRL_USE_LINUX_TIME

// air/OS: system_time() is the kernel's monotonic clock in microseconds,
// one call without a timespec to convert.
extern "C" long system_time(void); // OS.h: bigtime_t (long on 64-bit)

namespace crl::details {

void init() {
}

inner_time_type current_value() {
	return inner_time_type(system_time() / 1000);
}

time convert(inner_time_type value) {
	return time(value);
}

inner_profile_type current_profile_value() {
	return inner_profile_type(system_time());
}

profile_time convert_profile(inner_profile_type value) {
	return profile_time(value);
}

} // namespace crl::details

#endif // CRL_USE_LINUX_TIME
