// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
// Include Be API headers between base_haiku_begin.h and base_haiku_end.h.
//
// Desktop App Toolkit's int64/uint64 are qint64/quint64 (long long) and
// Haiku's SupportDefs.h declares its own int64/uint64 (long) in the global
// namespace too. Between the two headers Haiku's are named haiku_int64 and
// haiku_uint64; Be API calls that take them by pointer need those names.
#ifdef BASE_HAIKU_HEADERS_OPEN
#error "base_haiku_begin.h included twice without base_haiku_end.h"
#endif
#define BASE_HAIKU_HEADERS_OPEN
#define int64 haiku_int64
#define uint64 haiku_uint64
