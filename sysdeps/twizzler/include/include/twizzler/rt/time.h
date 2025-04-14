#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Supported monotonicity levels
enum monotonicity {
  NonMonotonic,
  WeakMonotonic,
  StrongMonotonic,
};

/// Get time from the monotonic clock
extern struct duration twz_rt_get_monotonic_time(void);
/// Get time from the system clock
extern struct duration twz_rt_get_system_time(void);

#ifdef __cplusplus
}
#endif
