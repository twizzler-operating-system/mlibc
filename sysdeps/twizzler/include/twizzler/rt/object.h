#pragma once

#include "types.h"
#include "handle.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Result map_object call
struct map_result {
  /// Handle, if error is set to Success.
  struct object_handle handle;
  twz_error error;
};

/// Map with READ permission.
const map_flags MAP_FLAG_R = 1;
/// Map with WRITE permission.
const map_flags MAP_FLAG_W = 2;
/// Map with EXEC permission.
const map_flags MAP_FLAG_X = 4;
/// Persist changes on flush.
const map_flags MAP_FLAG_PERSIST = 8;
/// Allow the runtime to provide additional safety properties.
const map_flags MAP_FLAG_INDIRECT = 16;
/// Don't map a null page for the object.
const map_flags MAP_FLAG_NO_NULLPAGE = 32;


/// Mapping flags
typedef uint32_t release_flags;

/// Don't cache this handle on release.
const release_flags RELEASE_NO_CACHE = 1;

/// Create a new runtime (volatile, tied to this runtime) object.
extern struct objid_result twz_rt_create_rtobj(void);

/// Map an object with a given ID and flags.
extern struct map_result twz_rt_map_object(objid id, map_flags flags);
/// Release an object handle. After calling this, the handle may not be used.
extern void twz_rt_release_handle(struct object_handle *handle, release_flags flags);

typedef uint32_t object_cmd;

const object_cmd OBJECT_CMD_DELETE = 1;

/// Modify an object.
extern twz_error twz_rt_object_cmd(struct object_handle *handle, object_cmd cmd, uint64_t arg);

/// Update an object handle.
extern twz_error twz_rt_update_handle(struct object_handle *handle);

/// Given a pointer, find the start of the associated object. The returned pointer and the passed pointer p
/// are guaranteed to be in the same object, allowing pointer arithmetic.
extern void *twz_rt_locate_object_start(void *p);

/// Given a pointer, find the associated object. The returned pointer and the passed pointer p
/// are guaranteed to be in the same object, allowing pointer arithmetic.
extern struct object_handle twz_rt_get_object_handle(void *p);

/// Resolve an FOT entry, returning an object handle for the target object with at least valid_len bytes of
/// addressable memory.
extern struct map_result twz_rt_resolve_fot(struct object_handle *handle, uint64_t idx, size_t valid_len, map_flags flags);
/// Does the same as twz_rt_resolve_fot but optimizes for local pointers and avoids cloning handles if possible. Returns null on failure
/// with no error code. Callers should try the twz_rt_resolve_fot function if this one fails.
extern void *twz_rt_resolve_fot_local(void *start, uint64_t idx, size_t valid_len, map_flags flags);

/// Insert the given entry into the FOT, or return the existing entry if it already exists in this object's FOT.
/// Returns -1 on failure.
extern struct u32_result twz_rt_insert_fot(struct object_handle *handle, void *entry);

// Not intended for public use.
extern void __twz_rt_map_two_objects(objid id_1, map_flags flags_1, objid id_2, map_flags flags_2, struct map_result *res_1, struct map_result *res_2);
#ifdef __cplusplus
}
#endif
