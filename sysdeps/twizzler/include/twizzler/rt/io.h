#pragma once

#include "types.h"
#include "fd.h"

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

/// Io vec, a buffer and a len.
struct io_vec {
  /// Pointer to buffer.
  void *iov_base;
  /// Length of buffer in bytes.
  size_t iov_len;
};

/// Do vectored IO read.
extern struct io_result twz_rt_fd_preadv(descriptor fd, const struct io_vec *iovs, size_t nr_iovs, struct io_ctx *ctx);
/// Do vectored IO write.
extern struct io_result twz_rt_fd_pwritev(descriptor fd, const struct io_vec *iovs, size_t nr_iovs, struct io_ctx *ctx);

typedef uint32_t wait_kind;
const wait_kind WAIT_READ = 1;
const wait_kind WAIT_WRITE = 2;

/// Get a word and value to wait on for determining if reads or writes are available.
extern twz_error twz_rt_fd_waitpoint(descriptor fd, wait_kind ek, uint64_t **point, uint64_t val);

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
