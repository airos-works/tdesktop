#pragma once

#ifdef Q_OS_HAIKU

#define _SUPPORT_DEFS_H

#ifdef __x86_64__
#define int64 __haiku_int64
#ifdef USE_HAIKU_UINT64
#define uint64 __haiku_uint64
#endif
#else
#define int32 __haiku_int32
#define uint32 __haiku_uint32
#endif

typedef int32 status_t;
typedef uint32 type_code;
typedef uint32 perform_code;

typedef int64 bigtime_t;
typedef int64 nanotime_t;

#endif
