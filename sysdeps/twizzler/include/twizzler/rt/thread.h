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
extern twz_error twz_rt_futex_wait(_Atomic futex_word *ptr, futex_word expected, struct option_duration timeout);
/// Wake up up to max threads waiting on ptr. If max is set to FUTEX_WAKE_ALL, wake all threads.
extern twz_error twz_rt_futex_wake(_Atomic futex_word *ptr, int64_t max);
/// As twz_rt_futex_wake, but reports how many threads were actually woken.
///
/// The count is not a nicety: libstd's RwLock asks "did I wake a writer?" and, told no, wakes every
/// waiting reader as well rather than risk nobody making progress. With only an error code to go
/// on the answer was always "no", so every contended write-unlock woke the whole reader set.
///
/// Kept separate from twz_rt_futex_wake rather than replacing it, because that one is also called
/// from mlibc (sys_futex_wake) and from libcxx's atomic.cpp, and changing its return type would
/// mean rebuilding libcxx inside llvm-project for a value neither caller wants.
extern struct u32_result twz_rt_futex_wake_count(_Atomic futex_word *ptr, int64_t max);

/// Wake all threads instead of a maximum number
const int64_t FUTEX_WAKE_ALL = -1;

/// Yield the thread now.
extern void twz_rt_yield_now(void);
/// Set the name of the calling thread. Must be a C string.
extern void twz_rt_set_name(const char *name);
/// Get the name of the calling thread. A slice of length *len is filled, and *len is updated to
/// contain the length actually used. The result is also a C string.
extern void twz_rt_get_name(const void *tcb, char *name, size_t *len);
/// Sleep the calling thread for specified duration.
extern void twz_rt_sleep(struct duration dur);

/// Get a pointer to the calling thread's interrupt-generation word.
///
/// The word counts signal handlers that have run on this thread and that should interrupt a
/// blocking operation. A blocking call samples it on entry and reports "interrupted" once it has
/// moved. Because it is ordinary memory it can also be handed to a sleep as an extra wait operand,
/// which is what keeps a handler that ran just before the sleep from being slept through.
///
/// The pointer is stable for the lifetime of the thread, and only that thread may access the word.
/// Accesses must be atomic; the type is spelled plainly here only because the word is also passed
/// to the kernel as a thread-sync operand.
extern uint64_t *twz_rt_interrupt_word(void);

/// Record that a signal handler which should interrupt blocking operations has run on this thread.
///
/// This is deliberately not called for every signal. POSIX interrupts a blocking call only when a
/// handler is actually caught and `SA_RESTART` is clear; ignored signals and restarting handlers
/// leave the call alone. Only the layer holding the handler table -- libc -- knows which case
/// applies, so the runtime exports the mechanism and libc decides when to use it.
extern void twz_rt_interrupt_bump(void);

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
  void *tcb;
};

/// Sawn a thread. On success, that thread starts executing concurrently with this function's return.
extern struct spawn_result twz_rt_spawn_thread(struct spawn_args args);

/// Wait for a thread to exit, optionally timing out.
extern twz_error twz_rt_join_thread(thread_id id, struct option_duration timeout);

struct thread_info {
    thread_id id;
    void *tcb;
    objid objid;
};

const thread_id TWZ_RT_THREAD_ID_SELF = (thread_id)0xFFFFFFFF;
extern struct thread_info twz_rt_get_thread_info(thread_id id);

/// Post a signal to another thread of this compartment.
///
/// The signal is delivered to that thread specifically -- its handler runs there, not on the
/// caller -- and a target parked in a blocking wait is woken so it reaches a delivery point
/// promptly. Delivery is asynchronous: this returns once the signal is posted, not once the
/// handler has run. A thread inside a cross-compartment call does not take delivery until it
/// returns to its own security context.
///
/// `signal` is a signal number in 1..64. Pending signals coalesce: two posts of the same number
/// before delivery are one delivery, so this cannot express queued realtime signals.
///
/// Waking the target is not by itself enough to make its blocking call return `EINTR` -- see
/// twz_rt_interrupt_bump, which the layer owning the handler table calls when POSIX says the
/// call should be interrupted rather than restarted.
extern twz_error twz_rt_thread_signal(thread_id id, uint64_t signal);

/// Stack bounds of a thread.
struct stack_bounds {
  /// Lowest valid address of the stack. 0 if unknown.
  uintptr_t start;
  /// Length of the stack in bytes. 0 if unknown.
  size_t len;
};

/// Get the calling thread's stack bounds.
///
/// Thread stacks here are runtime allocations, not OS-visible mappings, so this is the only way
/// to learn them. A runtime that does not know this thread's bounds -- a compartment's main
/// thread, whose stack the monitor built, or a runtime that does not track stacks at all --
/// returns {0, 0}, which means "unknown", never "empty": callers must fall back to whatever they
/// did without bounds. Spawned-thread stacks are heap allocations with no guard page below
/// `start`, so a caller pacing recursion against these bounds must keep its own red zone.
extern struct stack_bounds twz_rt_get_stack_bounds(void);

#ifdef __cplusplus
}
#endif
