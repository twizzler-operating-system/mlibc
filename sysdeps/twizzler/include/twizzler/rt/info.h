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

/// Physical memory, in pages of `page_size` bytes. A separate call rather than fields of
/// `system_info`, which is returned by value and would change size under existing callers.
struct memory_info {
  /// Every frame the kernel manages.
  size_t total_pages;
  /// Frames on the kernel's free lists.
  size_t free_pages;
  /// Page size
  size_t page_size;
};

/// Get physical memory sizes. Unlike system_info, the free count changes, so do not cache it.
extern struct memory_info twz_rt_get_memory_info(void);

#ifdef __cplusplus
}
#endif
