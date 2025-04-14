#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Flags to get_random
typedef uint32_t get_random_flags;

/// Do not block when collecting random data
const get_random_flags GET_RANDOM_NON_BLOCKING = 1;

/// Collect up to len bytes of randomness, filling buf. Returns the number of bytes
/// of random data actually collected.
extern size_t twz_rt_get_random(char *buf, size_t len, get_random_flags flags);
#ifdef __cplusplus
}
#endif
