#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Mapping flags
typedef uint32_t map_flags;

/// Object handle
struct object_handle {
  /// ID for this handle
  objid id;
  /// Pointer to per-runtime info. The first 64-bits of this data must be an atomic u64 value used for reference counting.
  void *runtime_info;
  /// Pointer to start of object data.
  void *start;
  /// Pointer to object meta struct.
  void *meta;
  /// Mapping flags
  map_flags map_flags;
  /// Number of valid bytes after start pointer for this object handle, in multiples of of LEN_MUL
  uint32_t valid_len;
};

/// Multiplier to valid_len.
const size_t LEN_MUL = 0x1000;

#ifdef __cplusplus
}
#endif
