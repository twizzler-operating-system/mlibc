#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Allocation flags
typedef uint32_t alloc_flags;

/// Zero memory during operation
const alloc_flags ZERO_MEMORY = 1;

/// Allocate memory, zeroing it first if the flag is set.
extern void *twz_rt_malloc(size_t sz, size_t align, alloc_flags flags);
/// Deallocate memory. If ZERO_MEMORY is set, will clear the memory before freeing.
extern void twz_rt_dealloc(void *ptr, size_t sz, size_t align, alloc_flags flags);
/// Reallocate memory. If ZERO_MEMORY is set, will zero new memory before returning and zero to-be-freed memory before freeing.
extern void *twz_rt_realloc(void *ptr, size_t sz, size_t align, size_t new_size, alloc_flags flags);

#ifdef __cplusplus
}
#endif
