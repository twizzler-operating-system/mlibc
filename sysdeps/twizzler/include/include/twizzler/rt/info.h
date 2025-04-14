#pragma once

#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Information about the system
struct system_info {
  /// Supported monotonicity
  enum monotonicity clock_monotonicity;
  /// Number of CPUs (hardware threads)
  size_t available_parallelism;
  /// Page size
  size_t page_size;
};

/// Get system information
extern struct system_info twz_rt_get_sysinfo(void);

#ifdef __cplusplus
}
#endif
