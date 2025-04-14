#pragma once

#include<stdint.h>
#include"types.h"
#include<stdbool.h>
#include<stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Futex type, based on linux futex.
typedef uint32_t futex_word;

/// If *ptr == expected, wait until signal, optionally timing out.
extern bool twz_rt_futex_wait(_Atomic futex_word *ptr, futex_word expected, struct option_duration timeout);
/// Wake up up to max threads waiting on ptr. If max is set to FUTEX_WAKE_ALL, wake all threads.
extern bool twz_rt_futex_wake(_Atomic futex_word *ptr, int64_t max);

/// Wake all threads instead of a maximum number
const int64_t FUTEX_WAKE_ALL = -1;

/// Yield the thread now.
extern void twz_rt_yield_now(void);
/// Set the name of the calling thread.
extern void twz_rt_set_name(const char *name);
/// Sleep the calling thread for specified duration.
extern void twz_rt_sleep(struct duration dur);

/// TLS index, module ID and offset.
struct tls_index {
  size_t mod_id;
  size_t offset;
};

/// Resolve the TLS index and get back the TLS data pointer.
extern void *twz_rt_tls_get_addr(struct tls_index *index);

/// A TLS desc struct, with a resolver and value
struct tls_desc {
    // note: a function pointer typedef here seems to break bindgen.
    /// Pointer to resolver
    void *resolver;
    /// Value to pass to the resolver
    uint64_t value;
};

/// Resolver for tls_desc
extern void *twz_rt_tls_desc_resolve(struct tls_desc *arg);

/// Runtime-internal ID of a thread
typedef uint32_t thread_id;

/// Arguments to spawn
struct spawn_args {
  /// Size of stack to allocate
  size_t stack_size;
  /// Starting address
  uintptr_t start;
  /// Starting argument
  size_t arg;
};

/// Spawn result.
struct spawn_result {
  /// Thread id, if err is set to Success.
  thread_id id;
  twz_error err;
};

/// Sawn a thread. On success, that thread starts executing concurrently with this function's return.
extern struct spawn_result twz_rt_spawn_thread(struct spawn_args args);

/// Wait for a thread to exit, optionally timing out.
extern twz_error twz_rt_join_thread(thread_id id, struct option_duration timeout);

#ifdef __cplusplus
}
#endif
