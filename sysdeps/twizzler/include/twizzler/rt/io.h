#pragma once

#include "types.h"
#include "fd.h"
#include <sys/select.h>
#include <poll.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Type of whence values for seek.
typedef uint32_t whence;

/// Flags for IO operations
typedef uint32_t io_flags;

/// Non-blocking behavior specified. If the operation would block, return io_result with error set to WouldBlock instead.
const io_flags IO_NONBLOCKING = 1;
/// Peek at the data without updating the internal position pointer or consuming any stream data.
const io_flags IO_PEEK = 2;
/// Wait for all the data to be ready.
const io_flags IO_WAITALL = 4;
/// Process out of band data, if supported.
const io_flags IO_OOB = 8;

/// Seek offset from start of file
const whence WHENCE_START = 0;
/// Seek offset from end of file
const whence WHENCE_END = 1;
/// Seek offset from current fd position
const whence WHENCE_CURRENT = 2;

/// Optional offset. If value is FD_POS, use the file descriptor position.
typedef int64_t optional_offset;
const optional_offset FD_POS = -1;

/// Context for I/O operations.
struct io_ctx {
  // Flags for this I/O operation.
  io_flags flags;
  // Optional offset. If set to FD_POS, will use the internal fd offset.
  optional_offset offset;
  // Optional timeout. If flags contains NONBLOCKING, this argument is ignored.
  struct option_duration timeout;
};

enum endpoint_kind {
  Endpoint_Unspecified,
  Endpoint_Socket,
};

union endpoint_addrs {
    struct socket_address socket_addr;
};

/// Endpoint addresses, for example, socket address.
struct endpoint {
  enum endpoint_kind kind;
  union endpoint_addrs addr;
};

#ifndef _MLIBC_POSIX_IOVEC_H
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#endif

/// Read from a file. May read less than specified len.
extern struct io_result twz_rt_fd_pread(descriptor fd, void *buf, size_t len, struct io_ctx *ctx);
/// Write to a file. May write less than specified len.
extern struct io_result twz_rt_fd_pwrite(descriptor fd, const void *buf, size_t len, struct io_ctx *ctx);
/// Seek to a specified point in the file.
extern struct io_result twz_rt_fd_seek(descriptor fd, whence whence, int64_t offset);

/// Read from a file. May read less than specified len. Fill *ep with information about the source of the I/O (e.g. socket address).
extern struct io_result twz_rt_fd_pread_from(descriptor fd, void *buf, size_t len, struct io_ctx *ctx, struct endpoint *ep);
/// Write to a file. May write less than specified len. Send to specified endpoint (e.g. socket address).
extern struct io_result twz_rt_fd_pwrite_to(descriptor fd, const void *buf, size_t len, struct io_ctx *ctx, const struct endpoint *ep);

/// Do vectored IO read.
extern struct io_result twz_rt_fd_preadv(descriptor fd, const struct iovec *iovs, size_t nr_iovs, struct io_ctx *ctx);
/// Do vectored IO write.
extern struct io_result twz_rt_fd_pwritev(descriptor fd, const struct iovec *iovs, size_t nr_iovs, struct io_ctx *ctx);

typedef uint32_t wait_kind;
const wait_kind WAIT_READ = 1;
const wait_kind WAIT_WRITE = 2;

/// Get a word and value to wait on for determining if reads or writes are available.
extern twz_error twz_rt_fd_waitpoint(descriptor fd, wait_kind ek, uint64_t **point, uint64_t *val, _Bool *ready);

/// Select-like operation.
extern struct io_result twz_rt_fd_select(size_t nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct option_duration timeout);
extern struct io_result twz_rt_fd_poll(struct pollfd *fds, size_t nfds, struct option_duration timeout);

/// Filter kinds for a kevent registration. Values, flags, and the struct layout below all match
/// the BSD/libc kqueue ABI, so libc's kevent() is a straight pass-through to twz_rt_fd_kevent with
/// no translation. Only readable/writable readiness and userspace-triggered notification are
/// supported -- unlike BSD kqueue there is no vnode/proc/signal/timer filter. Any future filter
/// should take its BSD value.
typedef int16_t kevent_filter;
const kevent_filter EVFILT_READ = -1;
const kevent_filter EVFILT_WRITE = -2;
/// Userspace-triggered notification. `ident` is any caller-chosen value -- it is NOT a descriptor.
/// The registration fires when a later changelist entry for the same (ident, EVFILT_USER) sets
/// NOTE_TRIGGER in fflags. Unlike the readiness filters, EVFILT_USER is always clear-on-report: a
/// trigger is consumed when reported, so one NOTE_TRIGGER yields exactly one event.
const kevent_filter EVFILT_USER = -11;

/// fflags bit on an EVFILT_USER changelist entry: fire that registration. The BSD NOTE_FF*
/// fflags-arithmetic operators are not supported; an EVFILT_USER registration's fflags is stored at
/// EV_ADD time and reported back unmodified.
const uint32_t NOTE_TRIGGER = 0x01000000;

/// Flags for a kevent changelist/eventlist entry.
typedef uint16_t kevent_flags;
/// Add (or update) this registration.
const kevent_flags EV_ADD = 0x0001;
/// Remove this registration.
const kevent_flags EV_DELETE = 0x0002;
/// Enable a previously-disabled registration.
const kevent_flags EV_ENABLE = 0x0004;
/// Disable this registration without removing it.
const kevent_flags EV_DISABLE = 0x0008;
/// Remove this registration after it fires once.
const kevent_flags EV_ONESHOT = 0x0010;
/// Accepted and ignored. The readiness filters are always level-triggered, matching twz_rt_fd_poll
/// / twz_rt_fd_select; there is no edge-triggered mode. (EVFILT_USER is always clear-on-report
/// regardless of this flag.)
const kevent_flags EV_CLEAR = 0x0020;
/// Always emit an eventlist receipt for this changelist entry, even when it applied cleanly (see
/// EV_ERROR). Sizing eventlist to nchanges therefore fills it with receipts, which is what lets a
/// pure-registration call return without waiting.
const kevent_flags EV_RECEIPT = 0x0040;
/// Set on an eventlist entry that is a receipt for a changelist entry rather than a readiness
/// event. `data` holds an errno describing why the change failed, or 0 if it applied cleanly (only
/// possible when the change asked for EV_RECEIPT). Failed changes always produce one of these.
const kevent_flags EV_ERROR = 0x4000;

struct kevent {
  /// The identity being registered on: a descriptor for EVFILT_READ/EVFILT_WRITE, or an arbitrary
  /// caller-chosen value for EVFILT_USER.
  uintptr_t ident;
  kevent_filter filter;
  kevent_flags flags;
  uint32_t fflags;
  intptr_t data;
  void *udata;
  /// Unused. Present so this matches the BSD/libc `struct kevent` layout (64 bytes) exactly.
  uint64_t ext[4];
};

/// Create a kqueue file descriptor via twz_rt_fd_open(OpenKind_Kqueue, flags, NULL, 0).
///
/// Apply the nchanges entries in changelist to kq's persistent registration set (see EV_ADD /
/// EV_DELETE / EV_ENABLE / EV_DISABLE / EV_ONESHOT above), then wait for up to nevents currently
/// enabled registrations to become ready (or for timeout to expire), writing them into eventlist.
/// Returns the number of entries written into eventlist, which may include EV_ERROR receipts
/// describing changelist entries. Receipts are written first, and once eventlist is full the call
/// returns without waiting -- so a caller that sets EV_RECEIPT on every change and sizes eventlist
/// to nchanges gets a non-blocking apply-only call.
extern struct io_result twz_rt_fd_kevent(descriptor kq, const struct kevent *changelist, size_t nchanges, struct kevent *eventlist, size_t nevents, struct option_duration timeout);

/// Get a config value for register reg.
extern twz_error twz_rt_fd_get_config(descriptor fd, uint32_t reg, void *val, size_t len);
/// Set a config value for register reg. Setting a register may have side effects.
extern twz_error twz_rt_fd_set_config(descriptor fd, uint32_t reg, const void *val, size_t len);

const uint32_t IO_REGISTER_ADDR = 1;
const uint32_t IO_REGISTER_PEER = 2;
const uint32_t IO_REGISTER_SOCKET_FLAGS = 3;
const uint32_t IO_REGISTER_LINGER = 4;
const uint32_t IO_REGISTER_TTL = 5;
const uint32_t IO_REGISTER_READTIMEOUT = 6;
const uint32_t IO_REGISTER_WRITETIMEOUT = 7;

const uint32_t IO_REGISTER_STATUS = 8;
const uint32_t IO_REGISTER_SIGNAL = 9;

const uint32_t IO_REGISTER_TERMIOS = 10;

const uint32_t IO_REGISTER_MULTICAST_V4 = 11;
const uint32_t IO_REGISTER_MULTICAST_V6 = 12;
const uint32_t IO_REGISTER_MULTICAST_TTL_V4 = 13;
const uint32_t IO_REGISTER_IO_FLAGS = 14;

const uint32_t IO_REGISTER_WINSIZE = 15;

const uint64_t STATUS_FLAG_TERMINATED = (1ull << 32);
const uint64_t STATUS_FLAG_READY = (1ull << 33);

const uint32_t SOCKET_FLAGS_NODELAY = 1;
const uint32_t SOCKET_FLAGS_ONLYV6 = 2;
const uint32_t SOCKET_FLAGS_BROADCAST = 4;
const uint32_t SOCKET_FLAGS_MULTICAST_LOOP_V4 = 8;
const uint32_t SOCKET_FLAGS_MULTICAST_LOOP_V6 = 0x10;

#ifdef __cplusplus
}
#endif
