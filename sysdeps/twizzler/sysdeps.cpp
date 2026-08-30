#include "include/twizzler/error.h"
#include "include/twizzler/rt/info.h"
#include "include/twizzler/rt/types.h"
#include "include/twizzler/rt/thread.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <type_traits>

#include <mlibc-config.h>
#include <bits/ensure.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/socklen_t.h>
#include <mlibc/allocator.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <limits.h>
#include <stdlib.h>

#include "../../options/linux/include/sys/sysinfo.h"
#include <twizzler/rt/object.h>
#include <twizzler/rt/fd.h>
#include <twizzler/rt/io.h>
#include <twizzler/rt/alloc.h>
#include <twizzler/rt/core.h>
#include <twizzler/rt/thread.h>
#include <twizzler/rt/exec.h>
#include <mlibc/tcb.hpp>
#include <twizzler/rt/random.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

extern "C" {
bool _systrace = true;
void __twz_enable_libc_trace(void) {
    _systrace = false;
}
}
#include "sysdeps.h"


static inline int32_t objid_to_ino(unsigned __int128 id) {
    if (id == 1) {
        return 0;
    }
    
    uint64_t hi = (uint64_t)(id >> 64);
    uint64_t lo = (uint64_t)id;
    
    if (hi == (1ULL << 63)) {
        uint32_t ino = (uint32_t)(lo & ~(1ULL << 63));
        return (int32_t)ino;
    }
    
    return -1;  // Represents failure/None case
}



static int twz_errno_generic(uint64_t code) {
    switch(code) {
        case NOT_SUPPORTED: return ENOTSUP;
        case INTERNAL: return ENOTSUP;
        case WOULD_BLOCK: return EAGAIN;
        case TIMED_OUT: return ETIMEDOUT;
        case ACCESS_DENIED: return EACCES;
        case NO_SUCH_OPERATION: return ENOSYS;
        case INTERRUPTED: return EINTR;
        case IN_PROGRESS: return EINPROGRESS;
        default: return -1;
    }
}

static int twz_errno_argument(uint64_t code) {
    switch(code) {
        case INVALID_ARGUMENT: return EINVAL;
        case WRONG_TYPE: return EINVAL;
        case INVALID_ADDRESS: return EADDRNOTAVAIL;
        case BAD_HANDLE: return EBADF;
        default: return EINVAL;
    }
}

static int twz_errno_resource(uint64_t code) {
    switch(code) {
        case OUT_OF_MEMORY: return ENOMEM;
        case OUT_OF_NAMES: return EADDRINUSE;
        case OUT_OF_RESOURCES: return ENOMEM;
        case UNAVAILABLE: return EADDRNOTAVAIL;
        case BUSY: return EBUSY;
        case NOT_CONNECTED: return ENOTCONN;
        case UNREACHABLE: return EADDRNOTAVAIL;
        case REFUSED: return EREMOTE;
        case NON_ATOMIC: return ENOTSUP;
        default: return EINVAL;
    }
}

static int twz_errno_naming(uint64_t code) {
    switch(code) {
        case NOT_FOUND: return ENOENT;
        case ALREADY_BOUND: return EADDRINUSE;
        case ALREADY_EXISTS: return EEXIST;
        case WRONG_NAME_KIND: return EINVAL;
        case INVALID_NAME: return ENAMETOOLONG;
        case LINK_LOOP: return ELOOP;
        case NOT_EMPTY: return ENOTEMPTY;
        default: return EINVAL;
    }
}

static int twz_errno_object(uint64_t code) {
    switch(code) {
        case MAPPING_FAILED: return ENOMEM;
        case NOT_MAPPED: return EBADF;
        case INVALID_FOTE: return EINVAL;
        case INVALID_PTR: return EINVAL;
        case INVALID_META: return EINVAL;
        case BASETYPE_MISMATCH: return EINVAL;
        case NO_SUCH_OBJECT: return ENOENT;
        default: return EINVAL;
    }
}

static int twz_errno_io(uint64_t code) {
    switch(code) {
        case DATA_LOSS: return ENODATA;
        default: return EIO;
    }
}

static int twz_errno_security(uint64_t code) {
    switch(code) {
        case INVALID_KEY: return EINVAL;
        case INVALID_SCHEME: return EINVAL;
        case SIGNATURE_MISMATCH: return EINVAL;
        case INVALID_GATE: return EINVAL;
        case GATE_DENIED: return EPERM;
        default: return EACCES;
    }
}

int twz_error_errno(uint64_t err) {
	if (err == 0) {
		return 0;
	}
	uint64_t category = (err & ERROR_CATEGORY_MASK) >> ERROR_CATEGORY_SHIFT;
	uint64_t code = (err & ERROR_CODE_MASK) >> ERROR_CODE_SHIFT;
	switch(category) {
		case GENERIC_ERROR: return twz_errno_generic(code);
		case ARGUMENT_ERROR: return twz_errno_argument(code);
		case RESOURCE_ERROR: return twz_errno_resource(code);
		case NAMING_ERROR: return twz_errno_naming(code);
		case OBJECT_ERROR: return twz_errno_object(code);
		case IO_ERROR: return twz_errno_io(code);
		case SECURITY_ERROR: return twz_errno_security(code);
	}
	return -1;
}

extern "C" {
__attribute__((weak))
void frg_panic(const char *mstr) {
//	mlibc::sys_libc_log("mlibc: Call to frg_panic");
	mlibc::sys_libc_log(mstr);
	mlibc::sys_libc_panic();
}

bool
__aarch64_sme_accessible(void)
{
	return true;
}
}

#define STUB_ONLY { __ensure(!"STUB_ONLY function was called"); __builtin_unreachable(); }
#define UNUSED(x) (void)(x);

// Tick rate reported by both sys_times() and sysconf(_SC_CLK_TCK); the two must agree.
#define TWZ_CLK_TCK 100

#ifndef MLIBC_BUILDING_RTLD
extern "C" long __do_syscall_ret(unsigned long ret) {
	if(ret > -4096UL) {
		errno = -ret;
		return -1;
	}
	return ret;
}
#endif

#include<mlibc/rtld-config.hpp>
mlibc::RtldConfig rtldConfig;

extern "C" [[ gnu::visibility("default") ]]
const mlibc::RtldConfig &__dlapi_get_config() {
	return rtldConfig;
}

namespace mlibc {

void sys_libc_log(const char *message) {
	size_t len = strnlen(message, 4096);
	size_t count = 0;
	while(count < len) {
		ssize_t thiscount = 0;
		int ret = sys_write(2, message + count, len - count, &thiscount);
		if (ret < 0) {
			return;
		}
		count += thiscount;
	}
	sys_write(2, "\n", 1, nullptr);
}

void sys_libc_panic() {
	__builtin_trap();
}

int sys_tcb_set(void *pointer) {
    SYSTRACE("sys_tcb_set(pointer=%p)", pointer);
    mlibc::sys_libc_log("tried to set TCB from within Twizzler-managed libc");
	return 0;
}

int sys_anon_allocate(size_t size, void **pointer) {
    SYSTRACE("sys_anon_allocate(size=%ld, pointer=%p)", size, pointer);
    *pointer = twz_rt_malloc(size, 128, ZERO_MEMORY);
    SYSTRACE("sys_anon_allocate allocated %p", *pointer);
    if (*pointer == NULL) {
        return -1;
    }
    return 0;
}

int sys_anon_free(void *pointer, size_t size) {
    SYSTRACE("sys_anon_free(pointer=%p, size=%ld)", pointer, size);
    twz_rt_dealloc(pointer, size, 128, 0);
	return 0;
}

int sys_fadvise(int fd, off_t offset, off_t length, int advice) {
    SYSTRACE("sys_fadvise(fd=%d, offset=%ld, length=%ld, advice=%d)", fd, offset, length, advice);
    // TODO
	int result = 0;
	SYSTRACE("sys_fadvise returning %d", result);
	return result;
}

// ---------------------------------------------------------------------------
// Per-descriptor state that the runtime's fd ABI cannot carry for us. Indexed by descriptor
// and bounded by the RLIMIT_NOFILE that sys_getrlimit reports; descriptors outside the range
// fall back to conservative defaults rather than being tracked.
//
//  - socket_prot_table: the socket type. POSIX picks stream vs. datagram at socket() time, but
//    the runtime needs the protocol at bind/connect time (twz_rt_fd_reopen's socket_bind_info)
//    and nothing in the ABI carries it between the two. Without this, bind() on a SOCK_DGRAM
//    socket creates a TCP listener.
//
//  - socket_flags_table: a shadow of the socket flags word. IO_REGISTER_SOCKET_FLAGS cannot
//    currently be read back -- the runtime's socket get_config passes an unwrapped Result<u32>
//    to its write_data helper (reference/src/runtime/file/kinds/socket.rs), so a 4-byte read is
//    size-rejected. The shadow lets set/getsockopt touch individual flag bits without a
//    read-modify-write, and stays accurate because the libc is that register's only writer.
//
//  - fd_openflags_table: the O_RDONLY/O_WRONLY/O_RDWR access mode and whether the descriptor was
//    opened O_APPEND. Neither is stored by the runtime, and fcntl(F_GETFL) has to report them.
//
// FD_CLOEXEC is deliberately *not* in this list. It used to be, and a libc-side table was the
// wrong home for it: the runtime is what builds a child's inherited descriptor set (read_binds
// plus the spawn path), so a flag only this libc could see could not affect what the child got.
// It is now runtime state, reached through FD_CMD_GET_CLOEXEC/FD_CMD_SET_CLOEXEC below.
//
// All are plain arrays touched with relaxed atomics: each entry is only meaningful while its
// descriptor is open, and a descriptor cannot be concurrently opened and used.
#define TWZ_MAX_TRACKED_FD 1024
#define TWZ_SOCK_PROT_NONE 0
#define TWZ_SOCK_PROT_STREAM 1
#define TWZ_SOCK_PROT_DGRAM 2

static unsigned char socket_prot_table[TWZ_MAX_TRACKED_FD];
static uint32_t socket_flags_table[TWZ_MAX_TRACKED_FD];

// Encoding: bits 0-1 hold the access mode, TWZ_FD_TRACKED marks the entry as populated (so an
// untracked descriptor can default to O_RDWR), and TWZ_FD_APPEND records O_APPEND.
#define TWZ_FD_TRACKED 0x4
#define TWZ_FD_APPEND  0x8
static unsigned char fd_openflags_table[TWZ_MAX_TRACKED_FD];

static void fd_openflags_set(int fd, int flags) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return;
    unsigned char v = (unsigned char)((flags & 03) | TWZ_FD_TRACKED);
    if (flags & O_APPEND)
        v |= TWZ_FD_APPEND;
    __atomic_store_n(&fd_openflags_table[fd], v, __ATOMIC_RELAXED);
}

// Returns the open flags this libc knows about (access mode plus O_APPEND), defaulting to
// O_RDWR for descriptors that never passed through sys_openat.
static int fd_openflags_get(int fd) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return O_RDWR;
    unsigned char v = __atomic_load_n(&fd_openflags_table[fd], __ATOMIC_RELAXED);
    if (!(v & TWZ_FD_TRACKED))
        return O_RDWR;
    return (int)(v & 03) | ((v & TWZ_FD_APPEND) ? O_APPEND : 0);
}

// Close-on-exec lives in the runtime, which is the only party that can act on it. Note the
// absence of a TWZ_MAX_TRACKED_FD bound: unlike the tables above, this has no fixed-size array
// behind it, so a high-numbered descriptor is tracked like any other.
static void fd_cloexec_set(int fd, bool on) {
    uint32_t val = on ? 1 : 0;
    twz_rt_fd_cmd(fd, FD_CMD_SET_CLOEXEC, &val, NULL);
}

static bool fd_cloexec_get(int fd) {
    uint32_t val = 0;
    if (twz_rt_fd_cmd(fd, FD_CMD_GET_CLOEXEC, NULL, &val) != SUCCESS)
        return false;
    return val != 0;
}

static void socket_prot_set(int fd, int type) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return;
    unsigned char v = (type == SOCK_DGRAM) ? TWZ_SOCK_PROT_DGRAM : TWZ_SOCK_PROT_STREAM;
    __atomic_store_n(&socket_prot_table[fd], v, __ATOMIC_RELAXED);
}

// Drop every per-descriptor record, so a recycled descriptor does not inherit stale state.
static void fd_state_clear(int fd) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return;
    __atomic_store_n(&socket_prot_table[fd], TWZ_SOCK_PROT_NONE, __ATOMIC_RELAXED);
    __atomic_store_n(&socket_flags_table[fd], 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&fd_openflags_table[fd], 0, __ATOMIC_RELAXED);
}

// Defaults to stream for descriptors we never saw go through sys_socket, which preserves the
// old behavior for anything unexpected.
static enum prot_kind socket_prot_get(int fd) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return ProtKind_Stream;
    unsigned char v = __atomic_load_n(&socket_prot_table[fd], __ATOMIC_RELAXED);
    return (v == TWZ_SOCK_PROT_DGRAM) ? ProtKind_Datagram : ProtKind_Stream;
}

static int socket_type_get(int fd) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return SOCK_STREAM;
    unsigned char v = __atomic_load_n(&socket_prot_table[fd], __ATOMIC_RELAXED);
    return (v == TWZ_SOCK_PROT_DGRAM) ? SOCK_DGRAM : SOCK_STREAM;
}

static uint32_t socket_flags_shadow(int fd) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return 0;
    return __atomic_load_n(&socket_flags_table[fd], __ATOMIC_RELAXED);
}

static void socket_flags_shadow_store(int fd, uint32_t flags) {
    if (fd < 0 || fd >= TWZ_MAX_TRACKED_FD)
        return;
    __atomic_store_n(&socket_flags_table[fd], flags, __ATOMIC_RELAXED);
}

// bind()/connect() reopen the descriptor onto a fresh socket object, which drops any flags
// already pushed to the runtime. Replay the shadow so options set before bind survive it.
static void socket_flags_reapply(int fd) {
    uint32_t flags = socket_flags_shadow(fd);
    if (flags) {
        twz_rt_fd_set_config(fd, IO_REGISTER_SOCKET_FLAGS, &flags, sizeof(flags));
    }
}

int sys_open(const char *path, int flags, mode_t mode, int *fd) {
    SYSTRACE("sys_open(path=%s, flags=%d, mode=%o, fd=%p)", path, flags, mode, fd);
    int result = sys_openat(AT_FDCWD, path, flags, mode, fd);
    SYSTRACE("sys_open returning %d", result);
    return result;
}

int sys_openat(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
    SYSTRACE("sys_openat(dirfd=%d, path=%s, flags=%d, mode=%o, fd=%p)", dirfd, path, flags, mode, fd);

    (void)mode;
    if (dirfd != AT_FDCWD) {
        SYSTRACE("sys_openat: dirfd != AT_FDCWD");
        mlibc::sys_libc_log("sys_openat: dirfd != AT_FDCWD");
        return ENOSYS;
    }
    struct create_options co = {
        .id = 0,
        .kind = CREATE_KIND_EXISTING,
    };
    if (flags & O_CREAT) {
        if (flags & O_EXCL) {
            co.kind = CREATE_KIND_NEW;
        } else {
            co.kind = CREATE_KIND_EITHER;
        }
    }
    // The access mode is a 2-bit field, not a bitmask: O_RDONLY is 0, so it cannot be tested
    // with `flags & O_RDONLY`.
    uint32_t open_flags = 0;
    switch (flags & 03) {
        case O_RDONLY: open_flags |= OPEN_FLAG_READ; break;
        case O_WRONLY: open_flags |= OPEN_FLAG_WRITE; break;
        case O_RDWR:   open_flags |= OPEN_FLAG_READ | OPEN_FLAG_WRITE; break;
        default:
            SYSTRACE("sys_openat: bad access mode in flags=%d", flags);
            return EINVAL;
    }
    if (flags & O_TRUNC) {
        open_flags |= OPEN_FLAG_TRUNCATE;
    }
    if (flags & O_APPEND) {
        open_flags |= OPEN_FLAG_TAIL;
    }
    if (flags & O_NOFOLLOW) {
        // Make the link itself the object of the open rather than its target. This is what
        // sys_stat needs to implement lstat(); for a plain open() we reject a symlink result
        // below, per POSIX.
        open_flags |= OPEN_FLAG_SYMLINK;
    }
    // O_SEARCH/O_EXEC/O_PATH are all the same bit here, and want no data access.
    if (flags & O_SEARCH) {
        open_flags |= OPEN_FLAG_READ;
    }
    // args.name is a fixed NAME_DATA_MAX buffer, but PATH_MAX is larger.
    size_t path_len = strlen(path);
    if (path_len >= NAME_DATA_MAX) {
        SYSTRACE("sys_openat: path length %ld exceeds NAME_DATA_MAX", path_len);
        return ENAMETOOLONG;
    }
    struct open_info args = {
        .create = co,
        .flags = 0,
        .len = path_len,
        .name = {}
    };
    memcpy(&args.name, path, path_len + 1);
    struct open_result res = twz_rt_fd_open(OpenKind_Path, open_flags, &args, sizeof(args));
    if (res.err != SUCCESS) {
        return twz_error_errno(res.err);
    }

    // O_DIRECTORY and O_NOFOLLOW are both constraints on what we were allowed to open, and the
    // runtime resolves by name kind rather than checking them, so enforce them here.
    if (flags & (O_DIRECTORY | O_NOFOLLOW)) {
        struct fd_info info;
        if (twz_rt_fd_get_info(res.fd, &info)) {
            if ((flags & O_DIRECTORY) && info.kind != FdKind_Directory) {
                twz_rt_fd_close(res.fd);
                SYSTRACE("sys_openat returning ENOTDIR (O_DIRECTORY, kind=%d)", info.kind);
                return ENOTDIR;
            }
            // O_PATH opens are allowed to name a symlink; ordinary ones are not.
            if ((flags & O_NOFOLLOW) && !(flags & O_PATH) && info.kind == FdKind_SymLink) {
                twz_rt_fd_close(res.fd);
                SYSTRACE("sys_openat returning ELOOP (O_NOFOLLOW on a symlink)");
                return ELOOP;
            }
        }
    }

    if (flags & O_NONBLOCK) {
        io_flags ioflags = IO_NONBLOCKING;
        twz_rt_fd_set_config(res.fd, IO_REGISTER_IO_FLAGS, &ioflags, sizeof(ioflags));
    }

    // Record what fcntl(F_GETFL) will need to report, and hand O_CLOEXEC to the runtime, which
    // drops flagged descriptors when it builds a child's inherited set.
    fd_state_clear(res.fd);
    fd_openflags_set(res.fd, flags);
    fd_cloexec_set(res.fd, (flags & O_CLOEXEC) != 0);

    if(fd) {
        *fd = res.fd;
    }
    return 0;
}

int sys_close(int fd) {
    SYSTRACE("sys_close(fd=%d)", fd);
    fd_state_clear(fd);
    twz_rt_fd_close(fd);
    int result = 0;
    SYSTRACE("sys_close returning %d", result);
    return result;
}

int sys_fcntl(int fd, int cmd, va_list args, int *result) {
    SYSTRACE("sys_fcntl(fd=%d, cmd=%d, result=%p)", fd, cmd, result);
	if (!result)
		return EFAULT;
	*result = 0;

	// Validate the descriptor up front so every command below can assume it exists. The
	// negative check has to come first: the runtime's fd table does `fd.try_into().unwrap()`
	// from i32 to usize, which panics rather than erroring.
	struct fd_info info;
	if (fd < 0 || !twz_rt_fd_get_info(fd, &info)) {
		SYSTRACE("sys_fcntl returning EBADF");
		return EBADF;
	}

	switch(cmd) {
		case F_DUPFD:
		case F_DUPFD_CLOEXEC: {
			// F_DUPFD must return the lowest free descriptor at or above the caller's minimum.
			// FD_CMD_DUP2 is no help (it would clobber an open descriptor at exactly min_fd),
			// so use FD_CMD_DUP -- which returns the lowest free descriptor -- and keep hold of
			// the too-low results so they aren't handed back again, until one clears the bar.
			// Only free slots are ever taken, so this cannot disturb another thread's
			// descriptors.
			int min_fd = va_arg(args, int);
			if (min_fd < 0 || min_fd >= TWZ_MAX_TRACKED_FD)
				return EINVAL;
			// Worst case every descriptor below min_fd is free and has to be held.
			descriptor held[TWZ_MAX_TRACKED_FD];
			size_t nr_held = 0;
			int err = 0;
			descriptor dup_fd = -1;
			for (;;) {
				twz_error e = twz_rt_fd_cmd(fd, FD_CMD_DUP, NULL, &dup_fd);
				if (e != SUCCESS) {
					err = twz_error_errno(e);
					break;
				}
				if (dup_fd >= min_fd)
					break;
				if (nr_held == sizeof(held) / sizeof(held[0])) {
					// Every low descriptor we could hold is still below min_fd.
					twz_rt_fd_close(dup_fd);
					err = EMFILE;
					break;
				}
				held[nr_held++] = dup_fd;
			}
			for (size_t i = 0; i < nr_held; i++) {
				twz_rt_fd_close(held[i]);
			}
			if (err) {
				SYSTRACE("sys_fcntl(F_DUPFD) returning %d", err);
				return err;
			}
			fd_state_clear(dup_fd);
			fd_openflags_set(dup_fd, fd_openflags_get(fd));
			socket_prot_set(dup_fd, socket_type_get(fd));
			fd_cloexec_set(dup_fd, cmd == F_DUPFD_CLOEXEC);
			*result = dup_fd;
			SYSTRACE("sys_fcntl(F_DUPFD) returning 0, newfd=%d", dup_fd);
			return 0;
		}
		case F_GETFD:
			*result = fd_cloexec_get(fd) ? FD_CLOEXEC : 0;
			return 0;
		case F_SETFD: {
			int arg = va_arg(args, int);
			fd_cloexec_set(fd, (arg & FD_CLOEXEC) != 0);
			return 0;
		}
		case F_GETFL: {
			io_flags ioflags = 0;
			twz_error e = twz_rt_fd_get_config(fd, IO_REGISTER_IO_FLAGS, &ioflags, sizeof(ioflags));
			int flags = fd_openflags_get(fd);
			if (e == SUCCESS && (ioflags & IO_NONBLOCKING)) {
				flags |= O_NONBLOCK;
			}
			*result = flags;
			SYSTRACE("sys_fcntl(F_GETFL) returning 0, flags=%d", flags);
			return 0;
		}
		case F_SETFL: {
			// POSIX says the access mode and creation flags are ignored here, leaving
			// O_NONBLOCK and O_APPEND. Append is fixed at open time by this ABI
			// (OPEN_FLAG_TAIL), so a request to turn it on afterwards has to be refused: it is
			// what fdopen(fd, "a") does, and quietly succeeding would leave writes going to the
			// current offset instead of the end of the file.
			int arg = va_arg(args, int);
			if ((arg & O_APPEND) && !(fd_openflags_get(fd) & O_APPEND)) {
				SYSTRACE("sys_fcntl(F_SETFL) returning EINVAL (cannot add O_APPEND after open)");
				return EINVAL;
			}
			io_flags ioflags = 0;
			twz_error e = twz_rt_fd_get_config(fd, IO_REGISTER_IO_FLAGS, &ioflags, sizeof(ioflags));
			if (e != SUCCESS)
				return twz_error_errno(e);
			if (arg & O_NONBLOCK) {
				ioflags |= IO_NONBLOCKING;
			} else {
				ioflags &= ~IO_NONBLOCKING;
			}
			return twz_error_errno(
				twz_rt_fd_set_config(fd, IO_REGISTER_IO_FLAGS, &ioflags, sizeof(ioflags)));
		}
		case F_GETLK: {
			// There is no cross-compartment file locking, and a single writer per object is the
			// only case that exists, so report the range as unlocked rather than failing --
			// callers that require locks to work (SQLite) treat an error as fatal.
			struct flock *lk = va_arg(args, struct flock *);
			if (!lk)
				return EFAULT;
			lk->l_type = F_UNLCK;
			return 0;
		}
		case F_SETLK:
		case F_SETLKW: {
			struct flock *lk = va_arg(args, struct flock *);
			if (!lk)
				return EFAULT;
			if (lk->l_type != F_RDLCK && lk->l_type != F_WRLCK && lk->l_type != F_UNLCK)
				return EINVAL;
			SYSTRACE("sys_fcntl(F_SETLK): accepting lock without effect");
			return 0;
		}
		default:
			// F_GETOWN/F_SETOWN and the rest need signals or have no analogue. Returning 0 here
			// would tell the caller its request took effect.
			SYSTRACE("sys_fcntl returning EINVAL (unhandled cmd %d)", cmd);
			return EINVAL;
	}
}

int sys_dup2(int fd, int flags, int newfd) {
    SYSTRACE("sys_dup2(fd=%d, flags=%d, newfd=%d)", fd, flags, newfd);

    // Guard negative descriptors before they reach the runtime: its fd table does
    // `fd.try_into().unwrap()` from i32 to usize, which panics rather than erroring.
    if (fd < 0 || newfd < 0) {
        return EBADF;
    }

    // FD_CMD_DUP2 places the duplicate at a chosen descriptor and closes whatever was there,
    // which is exactly dup2's contract. FD_CMD_DUP cannot serve here: it returns the lowest
    // free descriptor, so it would only land on newfd by luck.
    descriptor target = newfd;
    descriptor out = -1;
    twz_error err = twz_rt_fd_cmd(fd, FD_CMD_DUP2, &target, &out);
    int result = twz_error_errno(err);
    if (result != 0) {
        SYSTRACE("sys_dup2 returning %d", result);
        return result;
    }

    // dup2(fd, fd) validates fd and changes nothing, so leave the recorded state alone.
    if (fd != newfd) {
        fd_state_clear(newfd);
        fd_openflags_set(newfd, fd_openflags_get(fd));
        socket_prot_set(newfd, socket_type_get(fd));
        fd_cloexec_set(newfd, (flags & O_CLOEXEC) != 0);
    }

    SYSTRACE("sys_dup2 returning 0");
    return 0;
}

int sys_read(int fd, void *buffer, size_t size, ssize_t *bytes_read) {
    SYSTRACE("sys_read(fd=%d, buffer=%p, size=%ld, bytes_read=%p)", fd, buffer, size, bytes_read);
   	struct io_ctx ctx = {
		.flags = 0,
		.offset = FD_POS,
		.timeout = NO_DURATION,
	};
	if (bytes_read != nullptr) {
		*bytes_read = 0;
	}
	struct io_result res = twz_rt_fd_pread((descriptor)fd, buffer, size, &ctx);
	if (res.err == SUCCESS) {
        SYSTRACE("sys_read: read %ld bytes", res.val);
		if (bytes_read != nullptr) {
			*bytes_read = (ssize_t)res.val;
		}
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_readv(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_read) {
    SYSTRACE("sys_readv(fd=%d, iovs=%p, iovc=%d, bytes_read=%p)", fd, iovs, iovc, bytes_read);
    if (bytes_read != nullptr) {
		*bytes_read = 0;
	}
	struct io_ctx ctx = {
		.flags = 0,
		.offset = FD_POS,
		.timeout = NO_DURATION,
	};
	struct io_result res = twz_rt_fd_preadv((descriptor)fd,
		iovs, (size_t)iovc, &ctx);
	if (res.err == SUCCESS) {
		if (bytes_read != nullptr)
			*bytes_read = (ssize_t)res.val;
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_writev(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_written) {
    SYSTRACE("sys_writev(fd=%d, iovs=%p, iovc=%d, bytes_written=%p)", fd, iovs, iovc, bytes_written);
    if (bytes_written != nullptr) {
		*bytes_written = 0;
	}
	struct io_ctx ctx = {
		.flags = 0,
		.offset = FD_POS,
		.timeout = NO_DURATION,
	};
	struct io_result res = twz_rt_fd_pwritev((descriptor)fd,
		iovs, (size_t)iovc, &ctx);
	if (res.err == SUCCESS) {
		if (bytes_written != nullptr)
			*bytes_written = (ssize_t)res.val;
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_write(int fd, const void *buffer, size_t size, ssize_t *bytes_written) {
    //SYSTRACE("write(%d, %p, %ld)", fd, buffer, size);
   	struct io_ctx ctx = {
		.flags = 0,
		.offset = FD_POS,
		.timeout = NO_DURATION,
	};
	if (bytes_written != nullptr) {
		*bytes_written = 0;
	}
	struct io_result res = twz_rt_fd_pwrite((descriptor)fd, buffer, size, &ctx);
	if (res.err == SUCCESS) {
		if (bytes_written != nullptr) {
			*bytes_written = (ssize_t)res.val;
		}
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_seek(int fd, off_t offset, int whenc, off_t *new_offset) {
    SYSTRACE("sys_seek(fd=%d, offset=%ld, whenc=%d, new_offset=%p)", fd, offset, whenc, new_offset);
    whence tw = 0;
    if (whenc == SEEK_SET) {
        tw = WHENCE_START;
    }
    if (whenc == SEEK_CUR) {
        tw = WHENCE_CURRENT;
    }
    if (whenc == SEEK_END) {
        tw = WHENCE_END;
    }
   	struct io_result res = twz_rt_fd_seek(fd, tw, offset);
	if (res.err == SUCCESS) {
		if (new_offset != nullptr) {
			*new_offset = (off_t)res.val;
		}
		return 0;
	}
	return twz_error_errno(res.err);
}

// Accepted without effect: Twizzler does not model file modes. The runtime reports a constant
// S_IRWXU|S_IRWXG|S_IRWXO for every file, so there is no stored mode to change and no enforcement
// that could act on one. Refusing instead makes cp/mv/install fail outright over a property that
// does not exist here.
int sys_chmod(const char *pathname, mode_t mode) {
    SYSTRACE("sys_chmod(pathname=%s, mode=%o): accepting without effect", pathname, mode);
	return 0;
}

int sys_fchmod(int fd, mode_t mode) {
    SYSTRACE("sys_fchmod(fd=%d, mode=%o): accepting without effect", fd, mode);
	return 0;
}

int sys_fchmodat(int fd, const char *pathname, mode_t mode, int flags) {
    SYSTRACE("sys_fchmodat(fd=%d, pathname=%s, mode=%o, flags=%d): accepting without effect",
             fd, pathname, mode, flags);
	return 0;
}

int sys_fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
    SYSTRACE("sys_fchownat(dirfd=%d, pathname=%s, owner=%d, group=%d, flags=%d)", dirfd, pathname, owner, group, flags);
	int result = ENOSYS;
	SYSTRACE("sys_fchownat returning %d", result);
	return result;
}

int sys_utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags) {
    SYSTRACE("sys_utimensat(dirfd=%d, pathname=%s, times=%p, flags=%d)", dirfd, pathname, times, flags);
	int result = ENOSYS;
	SYSTRACE("sys_utimensat returning %d", result);
	return result;
}

// Object data begins one page in: object_handle::start is the base of the object, despite the
// ABI header describing it as "start of object data". Confirmed against new_object_handle in the
// reference runtime, which sets start = slot * MAX_SIZE and meta = start + MAX_SIZE - LEN_MUL.
#define TWZ_OBJ_DATA_OFF 0x1000

// File-backed regions handed out by sys_vm_map, so sys_vm_unmap can release the right handle.
//
// Anonymous mappings are deliberately *not* recorded. They come from twz_rt_malloc, which this
// libc's own allocator is built on, so allocating a record for one would re-enter the allocator.
// Their existing leak on unmap is untouched by this code -- fixing it needs the allocator path
// looked at as its own change, not a rider on this one.
#define TWZ_VM_MAX_MAPS 128
static struct {
	void *base;
	size_t len;
	struct object_handle handle;
	unsigned char used;
} vm_maps[TWZ_VM_MAX_MAPS];
static unsigned char vm_maps_lock;

static void vm_maps_acquire(void) {
	while(__atomic_test_and_set(&vm_maps_lock, __ATOMIC_ACQUIRE))
		;
}

static void vm_maps_release(void) {
	__atomic_clear(&vm_maps_lock, __ATOMIC_RELEASE);
}

static bool vm_map_record(void *base, size_t len, const struct object_handle *h) {
	vm_maps_acquire();
	for(size_t i = 0; i < TWZ_VM_MAX_MAPS; i++) {
		if(!vm_maps[i].used) {
			vm_maps[i].base = base;
			vm_maps[i].len = len;
			vm_maps[i].handle = *h;
			vm_maps[i].used = 1;
			vm_maps_release();
			return true;
		}
	}
	vm_maps_release();
	return false;
}

// Takes the record covering exactly `base`, if there is one. Partial unmapping is not supported:
// a caller unmapping half a region gets no action rather than a released handle for the whole of
// it, which would leave the surviving half pointing at nothing.
static bool vm_map_take(void *base, struct object_handle *out) {
	vm_maps_acquire();
	for(size_t i = 0; i < TWZ_VM_MAX_MAPS; i++) {
		if(vm_maps[i].used && vm_maps[i].base == base) {
			*out = vm_maps[i].handle;
			vm_maps[i].used = 0;
			vm_maps_release();
			return true;
		}
	}
	vm_maps_release();
	return false;
}

static map_flags twz_prot_to_map_flags(int prot) {
	map_flags f = 0;
	if(prot & PROT_READ)
		f |= MAP_FLAG_R;
	if(prot & PROT_WRITE)
		f |= MAP_FLAG_W;
	if(prot & PROT_EXEC)
		f |= MAP_FLAG_X;
	return f;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags,
		int fd, off_t offset, void **window) {
    SYSTRACE("sys_vm_map(size=%ld, prot=%d, flags=%d, fd=%d, offset=%ld)", size, prot, flags, fd, offset);
	if(!size)
		return EINVAL;

	if(!(flags & MAP_ANON)) {
		if(fd < 0)
			return EBADF;
		if(offset < 0 || (offset % TWZ_OBJ_DATA_OFF))
			return EINVAL;
		// A private file mapping must keep its writes out of the file. Mapping the object with
		// write permission is shared, so there is no way to honour this yet; refusing beats
		// quietly giving the caller a shared mapping it believes is private.
		if((flags & MAP_PRIVATE) && (prot & PROT_WRITE))
			return ENOTSUP;

		struct fd_info info;
		if(!twz_rt_fd_get_info(fd, &info))
			return EBADF;
		// Only regular files have a stable address for their bytes. A pty or socket is backed by
		// an object too, but mapping one would expose ring-buffer state, not file contents.
		if(info.kind != FdKind_Regular)
			return ENODEV;

		struct map_result mr = twz_rt_map_object(info.id, twz_prot_to_map_flags(prot));
		if(mr.error != SUCCESS)
			return twz_error_errno(mr.error);

		size_t valid = (size_t)mr.handle.valid_len * LEN_MUL;
		if((uint64_t)offset + size > valid) {
			// One file is one object, so a range past the data area cannot be made contiguous
			// with whatever follows it. Refuse rather than hand back a short mapping.
			twz_rt_release_handle(&mr.handle, 0);
			return ENOTSUP;
		}

		void *base = (char *)mr.handle.start + TWZ_OBJ_DATA_OFF + offset;
		if(!vm_map_record(base, size, &mr.handle)) {
			twz_rt_release_handle(&mr.handle, 0);
			return ENOMEM;
		}
		*window = base;
		SYSTRACE("sys_vm_map returning 0, window=%p", *window);
		return 0;
	}

	if(fd != -1)
		return ENOTSUP;
	*window = twz_rt_malloc(size, 0x1000, ZERO_MEMORY);
    if (*window == NULL) {
        return ENOMEM;
    }
    return 0;
	/*
	if(offset % 4096)
		return EINVAL;
	if(size >= PTRDIFF_MAX)
		return ENOMEM;
#if defined(SYS_mmap2)
	auto ret = do_syscall(SYS_mmap2, hint, size, prot, flags, fd, offset/4096);
#else
	auto ret = do_syscall(SYS_mmap, hint, size, prot, flags, fd, offset);
#endif
	// TODO: musl fixes up EPERM errors from the kernel.
	if(int e = sc_error(ret); e)
		return e;
	*window = sc_ptr_result<void>(ret);
	return 0;
	*/
	return ENOSYS;
}

int sys_vm_unmap(void *pointer, size_t size) {
    SYSTRACE("sys_vm_unmap(pointer=%p, size=%ld)", pointer, size);
	struct object_handle h;
	if(vm_map_take(pointer, &h)) {
		twz_rt_release_handle(&h, 0);
		SYSTRACE("sys_vm_unmap returning 0 (released object mapping)");
		return 0;
	}
	// Anonymous mappings are not tracked (see vm_maps), so this still returns success having
	// freed nothing for them -- every such mapping leaks, as it did before this change. Left
	// alone deliberately: the anonymous path is what mlibc's own allocator sits on, and undoing
	// it belongs in a change that can be tested against the allocator rather than beside it.
	int result = 0;
	SYSTRACE("sys_vm_unmap returning %d (untracked region, nothing released)", result);
	return result;
}

int sys_vm_protect(void *pointer, size_t size, int prot) {
    SYSTRACE("sys_vm_protect(pointer=%p, size=%ld, prot=%d)", pointer, size, prot);
	// Protections are fixed when a region is mapped and there is no call to change them in
	// place. Requests that only *drop* permissions are accepted: the memory stays more
	// permissive than asked, which no correct caller can detect. Adding execute is refused,
	// because a caller told it succeeded will jump into memory that is not executable and take
	// a fault far from here.
	if(prot & PROT_EXEC) {
		SYSTRACE("sys_vm_protect returning ENOSYS (cannot add PROT_EXEC after mapping)");
		return ENOSYS;
	}
	int result = 0;
	SYSTRACE("sys_vm_protect returning %d", result);
	return result;
}

// All remaining functions are disabled in ldso.

int sys_clock_get(int clock, time_t *secs, long *nanos) {
    SYSTRACE("sys_clock_get(clock=%d, secs=%p, nanos=%p)", clock, secs, nanos);
    
    if (!secs || !nanos) {
        return EFAULT;
    }
    
    struct duration dur;
    
    switch (clock) {
        case CLOCK_REALTIME:
        case CLOCK_REALTIME_COARSE:
            dur = twz_rt_get_system_time();
            break;
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_COARSE:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_BOOTTIME:
        //case CLOCK_UPTIME:
        //case CLOCK_UPTIME_RAW:
            dur = twz_rt_get_monotonic_time();
            break;
        case CLOCK_PROCESS_CPUTIME_ID:
        case CLOCK_THREAD_CPUTIME_ID:
            // No CPU-time accounting is exposed by the runtime, so elapsed time stands in.
            // Approximate, but callers that only need a monotonic timer (most profiling and
            // benchmarking code) work, where EINVAL would fail them outright.
            dur = twz_rt_get_monotonic_time();
            break;
        default:
            SYSTRACE("sys_clock_get: unsupported clock %d", clock);
            return EINVAL;
    }
    
    *secs = (time_t)dur.seconds;
    *nanos = (long)dur.nanos;
    
    SYSTRACE("sys_clock_get returning 0 (secs=%ld, nanos=%ld)", *secs, *nanos);
    return 0;
}

int sys_thread_getname(void *tcb, char *name, size_t len) {
    SYSTRACE("sys_thread_getname(tcb=%p, name=%p, len=%ld)", tcb, name, len);
    twz_rt_get_name(tcb, name, &len);
    return 0;
}

int sys_thread_setname(void *tcb, const char *name) {
    SYSTRACE("sys_thread_setname(tcb=%p, name=%s)", tcb, name);
    (void)tcb;
    twz_rt_set_name(name);
    return 0;
}

int sys_clock_getres(int clock, time_t *secs, long *nanos) {
    SYSTRACE("sys_clock_getres(clock=%d, secs=%p, nanos=%p)", clock, secs, nanos);
    
    if (!secs || !nanos) {
        return EFAULT;
    }
    
    // Report clock resolution based on the clock type
    // Most Twizzler clocks have nanosecond resolution
    switch (clock) {
        case CLOCK_REALTIME:
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_BOOTTIME:
        case CLOCK_PROCESS_CPUTIME_ID:
        case CLOCK_THREAD_CPUTIME_ID:
        //case CLOCK_UPTIME:
            *secs = 0;
            *nanos = 1;  // 1 nanosecond resolution
            break;
        case CLOCK_REALTIME_COARSE:
        case CLOCK_MONOTONIC_COARSE:
            *secs = 0;
            *nanos = 1000000;  // 1 millisecond resolution for coarse clocks
            break;
        //case CLOCK_UPTIME_RAW:
        //    *secs = 0;
        //    *nanos = 1;  // 1 nanosecond resolution
        //    break;
        default:
            SYSTRACE("sys_clock_getres: unsupported clock %d", clock);
            return EINVAL;
    }
    
    SYSTRACE("sys_clock_getres returning 0 (secs=%ld, nanos=%ld)", *secs, *nanos);
    return 0;
}

int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
    SYSTRACE("sys_stat(fsfdt=%d, fd=%d, path=%s, flags=%d, statbuf=%p)", fsfdt, fd, path, flags, statbuf);
    int oflags = O_RDONLY;
    if(flags & AT_SYMLINK_NOFOLLOW) {
        oflags |= O_NOFOLLOW;
    }

    // Track whether the descriptor is ours, so we don't leak one per stat() call.
    bool opened_fd = false;
    if (fsfdt == mlibc::fsfd_target::fd_path) {
        int e = sys_openat(fd, path, oflags, 0, &fd);
        if (e != 0) {
            return e;
        }
        opened_fd = true;
    } else if (fsfdt == mlibc::fsfd_target::path) {
        int e = sys_openat(AT_FDCWD, path, oflags, 0, &fd);
        if (e != 0) {
            return e;
        }
        opened_fd = true;
    }
    struct fd_info info = {};
    bool got_info = twz_rt_fd_get_info(fd, &info);
    if (opened_fd) {
        twz_rt_fd_close(fd);
    }
    if (!got_info) {
        return EBADF;
    }
    SYSTRACE("sys_stat: got fd info: mode=%o, len=%ld", info.unix_mode, info.len);

    // Callers hand us an uninitialized struct stat, and POSIX requires every field to be
    // written. An unwritten field is not cosmetic here: LLVM's
    // raw_fd_ostream::preferred_buffer_size() returns st_blksize verbatim and then does
    // `new char[that]`, so the caller's own stack garbage became a ~347GB request that wedged
    // rustc in the allocator's grow-retry loop. Zero first, then fill -- that also covers
    // st_blocks, the timespec nanosecond halves, and any field the ABI grows later.
    memset(statbuf, 0, sizeof(*statbuf));
    statbuf->st_ino = objid_to_ino(info.id);
    // The runtime reports a full mode (type bits | permissions). Only synthesize one if it
    // didn't fill the field in, otherwise we'd clobber the real permissions.
    statbuf->st_mode = info.unix_mode;
    if ((statbuf->st_mode & S_IFMT) == 0) {
        switch (info.kind) {
            case FdKind_Directory: statbuf->st_mode = S_IFDIR | 0755; break;
            case FdKind_SymLink:   statbuf->st_mode = S_IFLNK | 0777; break;
            case FdKind_Socket:    statbuf->st_mode = S_IFSOCK | 0777; break;
            case FdKind_Pipe:      statbuf->st_mode = S_IFIFO | 0666; break;
            case FdKind_Pty:       statbuf->st_mode = S_IFCHR | 0666; break;
            default:               statbuf->st_mode = S_IFREG | 0777; break;
        }
    }
    statbuf->st_nlink = 1;
    statbuf->st_size = info.len;
    statbuf->st_blksize = 0x1000;
    statbuf->st_blocks = (blkcnt_t)((info.len + 511) / 512);
    statbuf->st_atim.tv_sec = info.accessed.seconds;
    statbuf->st_atim.tv_nsec = info.accessed.nanos;
    statbuf->st_mtim.tv_sec = info.modified.seconds;
    statbuf->st_mtim.tv_nsec = info.modified.nanos;
    statbuf->st_ctim.tv_sec = info.created.seconds;
    statbuf->st_ctim.tv_nsec = info.created.nanos;
    return 0;
}

// Left ENOSYS deliberately: mlibc's linux_option is off for this build, so <sys/statfs.h> is not
// installed and `struct statfs` is an incomplete type here -- it cannot be filled in, and no
// userspace caller can reach statfs() without the header anyway.
int sys_statfs(const char *path, struct statfs *buf) {
    SYSTRACE("sys_statfs(path=%s, buf=%p)", path, buf);
    sys_libc_log("call to statfs");
	return ENOSYS;
}

int sys_fstatfs(int fd, struct statfs *buf) {
    SYSTRACE("sys_fstatfs(fd=%d, buf=%p)", fd, buf);
    sys_libc_log("call to fstatfs");
	return ENOSYS;
}

extern "C" void __mlibc_signal_restore(void);
extern "C" void __mlibc_signal_restore_rt(void);

// Signal support.
//
// Twizzler keeps no in-kernel disposition table. The monitor posts a signal to a compartment, the
// kernel turns it into a Mailbox upcall on the target thread, and the reference runtime's default
// upcall handler calls __mlibc_handle_signal() below. That upcall interrupts whatever the thread
// was doing, so nothing on the delivery path may take a lock: a thread interrupted inside
// sigaction() would deadlock against itself. The table is read lock-free instead -- writers
// publish the handler pointer with a release store *after* the flags and mask it selects, and the
// dispatcher acquire-loads it.
//
// _NSIG is 65, so every signal fits in sigset_t::__sig[0] and masks are single-word atomics.

// Return values for __mlibc_handle_signal(). Mirrored in the reference runtime's upcall handler
// (src/rt/reference/src/runtime/upcall.rs) -- keep the two in sync.
#define MLIBC_SIGNAL_DEFAULT 0
#define MLIBC_SIGNAL_HANDLED 1
#define MLIBC_SIGNAL_IGNORED 2

namespace {

struct SigDisposition {
	uintptr_t handler; // SIG_DFL, SIG_IGN, or a function pointer. Published last.
	unsigned long flags;
	unsigned long mask;
};

SigDisposition sigDispositions[NSIG];
unsigned long sigBlocked;
unsigned long sigPending;
// Bumped once per delivery so pause()/sigsuspend() can tell that something arrived.
futex_word sigDeliveries;

constexpr unsigned long sigBit(int sig) {
	return 1UL << (sig - 1);
}

bool sigValid(int sig) {
	return sig > 0 && sig < NSIG;
}

unsigned long sigMaskOf(const sigset_t *set) {
	return set ? set->__sig[0] : 0;
}

void sigSetMask(sigset_t *set, unsigned long mask) {
	memset(set, 0, sizeof(*set));
	set->__sig[0] = mask;
}

// Defined by the reference runtime (src/rt/reference/src/runtime/upcall.rs), which owns the single
// table of what an unhandled signal does -- keeping it there means the upcall path and the
// raise-against-self path below cannot drift apart. Weak, because a runtime that predates it (or
// the minimal runtime, which has no upcall handling) still has to leave us something sane.
extern "C" __attribute__((weak)) void __twz_rt_default_signal_action(int sig);

void sigDefaultAction(int sig) {
	if(__twz_rt_default_signal_action) {
		__twz_rt_default_signal_action(sig);
		return;
	}
	twz_rt_exit(128 + sig);
}

// Runs the installed disposition for sig. Deliberately does not chain to pending signals; callers
// do that once at top level so recursion depth stays bounded.
int sigDeliver(int sig) {
	if(!sigValid(sig))
		return MLIBC_SIGNAL_DEFAULT;

	unsigned long bit = sigBit(sig);
	if(__atomic_load_n(&sigBlocked, __ATOMIC_ACQUIRE) & bit) {
		__atomic_fetch_or(&sigPending, bit, __ATOMIC_ACQ_REL);
		return MLIBC_SIGNAL_HANDLED;
	}

	SigDisposition *disp = &sigDispositions[sig];
	uintptr_t handler = __atomic_load_n(&disp->handler, __ATOMIC_ACQUIRE);
	if(handler == reinterpret_cast<uintptr_t>(SIG_DFL))
		return MLIBC_SIGNAL_DEFAULT;
	if(handler == reinterpret_cast<uintptr_t>(SIG_IGN))
		return MLIBC_SIGNAL_IGNORED;

	unsigned long flags = disp->flags;
	unsigned long blockDuringHandler = disp->mask;
	if(!(flags & SA_NODEFER))
		blockDuringHandler |= bit;
	if(flags & SA_RESETHAND)
		__atomic_store_n(&disp->handler, reinterpret_cast<uintptr_t>(SIG_DFL), __ATOMIC_RELEASE);

	unsigned long saved = __atomic_fetch_or(&sigBlocked, blockDuringHandler, __ATOMIC_ACQ_REL);
	if(flags & SA_SIGINFO) {
		siginfo_t info;
		memset(&info, 0, sizeof(info));
		info.si_signo = sig;
		info.si_code = SI_USER;
		reinterpret_cast<void (*)(int, siginfo_t *, void *)>(handler)(sig, &info, nullptr);
	} else {
		reinterpret_cast<void (*)(int)>(handler)(sig);
	}
	// POSIX restores the mask the handler ran under when it returns.
	__atomic_store_n(&sigBlocked, saved, __ATOMIC_RELEASE);
	return MLIBC_SIGNAL_HANDLED;
}

void sigNoteDelivery() {
	__atomic_add_fetch(&sigDeliveries, 1, __ATOMIC_ACQ_REL);
	twz_rt_futex_wake(reinterpret_cast<_Atomic futex_word *>(&sigDeliveries), FUTEX_WAKE_ALL);
}

// Deliver anything queued that is no longer blocked.
void sigDrainPending() {
	for(;;) {
		unsigned long blocked = __atomic_load_n(&sigBlocked, __ATOMIC_ACQUIRE);
		unsigned long ready = __atomic_load_n(&sigPending, __ATOMIC_ACQUIRE) & ~blocked;
		if(!ready)
			return;
		int sig = __builtin_ctzl(ready) + 1;
		__atomic_fetch_and(&sigPending, ~sigBit(sig), __ATOMIC_ACQ_REL);
		if(sigDeliver(sig) == MLIBC_SIGNAL_DEFAULT)
			sigDefaultAction(sig);
		sigNoteDelivery();
	}
}

// Raise a signal against this process, applying the default action here. Used by
// raise()/abort()/kill(self), where there is no runtime upcall to fall back to.
void sigRaiseSelf(int sig) {
	int result = sigDeliver(sig);
	sigNoteDelivery();
	if(result == MLIBC_SIGNAL_DEFAULT)
		sigDefaultAction(sig);
	sigDrainPending();
}

// A signal aimed at this thread arrives as an upcall that interrupts the wait itself, so the sleep
// is bounded rather than indefinite: the handler runs, the counter moves, and the next iteration
// observes it.
void sigWaitForDelivery(futex_word expected) {
	struct option_duration timeout = NO_DURATION;
	timeout.dur.seconds = 0;
	timeout.dur.nanos = 50 * 1000 * 1000;
	timeout.is_some = 1;
	twz_rt_futex_wait(reinterpret_cast<_Atomic futex_word *>(&sigDeliveries), expected, timeout);
}

} // namespace

// Called by the reference runtime's default upcall handler on a Mailbox upcall. It resolves this
// as a weak symbol, so a compartment that does not link mlibc keeps the runtime's own behavior.
//
// Returns MLIBC_SIGNAL_HANDLED if a handler ran (or the signal was blocked and is now pending),
// MLIBC_SIGNAL_IGNORED if the program explicitly installed SIG_IGN, and MLIBC_SIGNAL_DEFAULT if no
// disposition is installed -- in which case the runtime applies its own default action.
extern "C" int __mlibc_handle_signal(int sig) {
	int result = sigDeliver(sig);
	sigNoteDelivery();
	sigDrainPending();
	return result;
}

int sys_sigaction(int signum, const struct sigaction *act,
		struct sigaction *oldact) {
	SYSTRACE("sys_sigaction(signum=%d, act=%p, oldact=%p)", signum, act, oldact);
	if(!sigValid(signum))
		return EINVAL;
	// SIGKILL and SIGSTOP dispositions cannot be changed, but may still be queried.
	if(act && (signum == SIGKILL || signum == SIGSTOP))
		return EINVAL;

	SigDisposition *disp = &sigDispositions[signum];
	if(oldact) {
		uintptr_t handler = __atomic_load_n(&disp->handler, __ATOMIC_ACQUIRE);
		oldact->sa_flags = disp->flags;
		sigSetMask(&oldact->sa_mask, disp->mask);
		// sa_handler and sa_sigaction alias the same union member.
		oldact->sa_handler = reinterpret_cast<void (*)(int)>(handler);
	}
	if(act) {
		uintptr_t handler = (act->sa_flags & SA_SIGINFO)
			? reinterpret_cast<uintptr_t>(act->sa_sigaction)
			: reinterpret_cast<uintptr_t>(act->sa_handler);
		// Flags and mask must be visible before the handler pointer that selects them.
		disp->flags = act->sa_flags;
		disp->mask = sigMaskOf(&act->sa_mask);
		__atomic_store_n(&disp->handler, handler, __ATOMIC_RELEASE);
		// Installing SIG_IGN discards anything already queued for this signal.
		if(handler == reinterpret_cast<uintptr_t>(SIG_IGN))
			__atomic_fetch_and(&sigPending, ~sigBit(signum), __ATOMIC_ACQ_REL);
	}
	return 0;
}

int sys_sigpending(sigset_t *set) {
	SYSTRACE("sys_sigpending(set=%p)", set);
	if(!set)
		return EINVAL;
	sigSetMask(set, __atomic_load_n(&sigPending, __ATOMIC_ACQUIRE));
	return 0;
}

int sys_pause() {
	SYSTRACE("sys_pause()");
	futex_word start = __atomic_load_n(&sigDeliveries, __ATOMIC_ACQUIRE);
	for(;;) {
		sigDrainPending();
		if(__atomic_load_n(&sigDeliveries, __ATOMIC_ACQUIRE) != start)
			return EINTR;
		sigWaitForDelivery(start);
	}
}

int sys_sigsuspend(const sigset_t *set) {
	SYSTRACE("sys_sigsuspend(set=%p)", set);
	unsigned long saved = __atomic_load_n(&sigBlocked, __ATOMIC_ACQUIRE);
	unsigned long wanted = sigMaskOf(set) & ~(sigBit(SIGKILL) | sigBit(SIGSTOP));
	__atomic_store_n(&sigBlocked, wanted, __ATOMIC_RELEASE);
	int result = sys_pause();
	__atomic_store_n(&sigBlocked, saved, __ATOMIC_RELEASE);
	sigDrainPending();
	return result;
}

// Helper: Convert POSIX sockaddr to Twizzler socket_address
static int sockaddr_to_twz_addr(const struct sockaddr *sa, socklen_t len,
    struct socket_address &out_addr) {
    if (!sa || len < sizeof(sa_family_t)) return EINVAL;
    
    switch (sa->sa_family) {
        case AF_INET: {
            if (len < sizeof(struct sockaddr_in)) return EINVAL;
            struct sockaddr_in *sin = (struct sockaddr_in *)sa;
            out_addr.kind = AddrKind_Ipv4;
            out_addr.port = ntohs(sin->sin_port);
            memcpy(out_addr.addr_octets.v4, &sin->sin_addr, 4);
            out_addr.flowinfo = 0;
            out_addr.scope_id = 0;
            return 0;
        }
        case AF_INET6: {
            if (len < sizeof(struct sockaddr_in6)) return EINVAL;
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
            out_addr.kind = AddrKind_Ipv6;
            out_addr.port = ntohs(sin6->sin6_port);
            memcpy(out_addr.addr_octets.v6, &sin6->sin6_addr, 16);
            out_addr.flowinfo = sin6->sin6_flowinfo;
            out_addr.scope_id = sin6->sin6_scope_id;
            return 0;
        }
        default:
            return EAFNOSUPPORT;
    }
}

int sys_socket(int domain, int type, int protocol, int *fd) {
    SYSTRACE("sys_socket(domain=%d, type=%d, protocol=%d, fd=%p)", domain, type, protocol, fd);
    
    if (!fd) return EFAULT;
    if (domain != AF_INET && domain != AF_INET6) return EAFNOSUPPORT;
    int nonblock = type & SOCK_NONBLOCK;
    type = type & ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
    SYSTRACE("sys_socket: new type=%d, nonblock=%d", type, nonblock);
    if (type != SOCK_STREAM && type != SOCK_DGRAM) return EINVAL;
    
    // Create an unbound socket
    struct open_result res = twz_rt_fd_open(OpenKind_SocketBind, OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        NULL, 0);

    if (res.err != SUCCESS) {
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_socket returning %d", result);
        return result;
    }

    if (nonblock) {
        io_flags flags = IO_NONBLOCKING;
        twz_rt_fd_set_config(res.fd, IO_REGISTER_IO_FLAGS, &flags, sizeof(flags));
    }

    // Remember the type so bind()/connect() can pass the right prot_kind.
    socket_prot_set(res.fd, type);

    *fd = res.fd;
    SYSTRACE("sys_socket returning 0, fd=%d", *fd);
    return 0;
}

int sys_msg_send(int sockfd, const struct msghdr *msg, int flags, ssize_t *length) {
    SYSTRACE("sys_msg_send(sockfd=%d, msg=%p, flags=%d, length=%p)", sockfd, msg, flags, length);
    
    if (!msg) return EFAULT;
    if (length) *length = 0;
    
    ssize_t total_sent = 0;
    struct io_ctx ctx = {
        .flags = 0,
        .offset = FD_POS,
        .timeout = NO_DURATION,
    };
    
    // Process each iovec in the message
    for (int i = 0; i < msg->msg_iovlen; i++) {
        const struct iovec *iov = &msg->msg_iov[i];
        if (!iov->iov_base || iov->iov_len == 0) continue;
        
        struct io_result res = twz_rt_fd_pwrite((descriptor)sockfd, iov->iov_base, iov->iov_len, &ctx);
        
        if (res.err != SUCCESS) {
            // A short write is still a success; the count goes out via *length.
            if (total_sent > 0) {
                if (length) *length = total_sent;
                SYSTRACE("sys_msg_send returning 0 (partial, %ld bytes)", total_sent);
                return 0;
            }
            int result = twz_error_errno(res.err);
            SYSTRACE("sys_msg_send returning %d", result);
            return result;
        }

        total_sent += res.val;
    }

    if (length) *length = total_sent;
    SYSTRACE("sys_msg_send returning 0 (%ld bytes)", total_sent);
    // mlibc's contract is 0-or-errno, not a byte count.
    return 0;
}

ssize_t sys_sendto(int fd, const void *buffer, size_t size, int flags, const struct sockaddr *sock_addr, socklen_t addr_length, ssize_t *length) {
    SYSTRACE("sys_sendto(fd=%d, buffer=%p, size=%ld, flags=%d, sock_addr=%p, addr_length=%d, length=%p)", fd, buffer, size, flags, sock_addr, addr_length, length);
    
    if (!buffer) return EFAULT;
    if (length) *length = 0;
    
    struct io_ctx ctx = {
        .flags = 0,
        .offset = FD_POS,
        .timeout = NO_DURATION,
    };
    
    // If address is provided, use pwrite_to to send to that address
    if (sock_addr && addr_length > 0) {
        SYSTRACE("sys_sendto: got address");
        struct socket_address twz_addr = {};
        int err = sockaddr_to_twz_addr(sock_addr, addr_length, twz_addr);
        if (err) {
            SYSTRACE("sys_sendto returning %d", err);
            return err;
        }
        
        struct endpoint ep = {
            .kind = Endpoint_Socket,
            .addr = {.socket_addr = twz_addr}
        };
        
        struct io_result res = twz_rt_fd_pwrite_to((descriptor)fd, buffer, size, &ctx, &ep);
        
        if (res.err == SUCCESS) {
            if (length) *length = (ssize_t)res.val;
            SYSTRACE("sys_sendto(ok) returning %ld", res.val);
            return 0;
        }
        
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_sendto(err) returning %d", result);
        return result;
    }
    
    SYSTRACE("sys_sendto: no address provided, using pwrite");
    // If no address, just write
    struct io_result res = twz_rt_fd_pwrite((descriptor)fd, buffer, size, &ctx);
    
    if (res.err == SUCCESS) {
        if (length) *length = (ssize_t)res.val;
        SYSTRACE("sys_sendto(ok) returning %ld", res.val);
        return 0;
    }
    
    int result = twz_error_errno(res.err);
    SYSTRACE("sys_sendto(err) returning %d", result);
    return result;
}

ssize_t sys_recvfrom(int fd, void *buffer, size_t size, int flags, struct sockaddr *sock_addr, socklen_t *addr_length, ssize_t *length) {
    SYSTRACE("sys_recvfrom(fd=%d, buffer=%p, size=%ld, flags=%d, sock_addr=%p, addr_length=%p, length=%p)", fd, buffer, size, flags, sock_addr, addr_length, length);
    
    if (!buffer) return EFAULT;
    if (length) *length = 0;
    
    struct io_ctx ctx = {
        .flags = 0,
        .offset = FD_POS,
        .timeout = NO_DURATION,
    };
    
    // If address buffer provided, use pread_from to get peer information
    if (sock_addr && addr_length && *addr_length > 0) {
        struct endpoint ep = {};
        struct io_result res = twz_rt_fd_pread_from((descriptor)fd, buffer, size, &ctx, &ep);
        
        if (res.err == SUCCESS) {
            if (length) *length = (ssize_t)res.val;
            
            // Convert endpoint socket address to POSIX sockaddr
            if (ep.kind == Endpoint_Socket) {
                struct socket_address *sa = &ep.addr.socket_addr;
                socklen_t needed_len = 0;
                
                if (sa->kind == AddrKind_Ipv4) {
                    needed_len = sizeof(struct sockaddr_in);
                    if (*addr_length >= needed_len) {
                        struct sockaddr_in *sin = (struct sockaddr_in *)sock_addr;
                        sin->sin_family = AF_INET;
                        sin->sin_port = htons(sa->port);
                        memcpy(&sin->sin_addr, sa->addr_octets.v4, 4);
                    }
                } else if (sa->kind == AddrKind_Ipv6) {
                    needed_len = sizeof(struct sockaddr_in6);
                    if (*addr_length >= needed_len) {
                        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sock_addr;
                        sin6->sin6_family = AF_INET6;
                        sin6->sin6_port = htons(sa->port);
                        memcpy(&sin6->sin6_addr, sa->addr_octets.v6, 16);
                        sin6->sin6_flowinfo = sa->flowinfo;
                        sin6->sin6_scope_id = sa->scope_id;
                    }
                }
                *addr_length = needed_len;
            }
            
            SYSTRACE("sys_recvfrom returning %ld", res.val);
            return 0;
        }
        
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_recvfrom returning %d", result);
        return result;
    }
    
    // No address buffer, just read
    struct io_result res = twz_rt_fd_pread((descriptor)fd, buffer, size, &ctx);
    
    if (res.err == SUCCESS) {
        if (length) *length = (ssize_t)res.val;
        SYSTRACE("sys_recvfrom returning %ld", res.val);
        return 0;
    }
    
    int result = twz_error_errno(res.err);
    SYSTRACE("sys_recvfrom returning %d", result);
    return result;
}

int sys_msg_recv(int sockfd, struct msghdr *msg, int flags, ssize_t *length) {
    SYSTRACE("sys_msg_recv(sockfd=%d, msg=%p, flags=%d, length=%p)", sockfd, msg, flags, length);
    
    if (!msg) return EFAULT;
    if (length) *length = 0;
    
    ssize_t total_read = 0;
    struct io_ctx ctx = {
        .flags = 0,
        .offset = FD_POS,
        .timeout = NO_DURATION,
    };
    
    // Process each iovec in the message
    for (int i = 0; i < msg->msg_iovlen; i++) {
        struct iovec *iov = &msg->msg_iov[i];
        if (!iov->iov_base || iov->iov_len == 0) continue;
        
        struct io_result res = twz_rt_fd_pread((descriptor)sockfd, iov->iov_base, iov->iov_len, &ctx);
        
        if (res.err != SUCCESS) {
            // A short read is still a success; the count goes out via *length.
            if (total_read > 0) {
                if (length) *length = total_read;
                SYSTRACE("sys_msg_recv returning 0 (partial, %ld bytes)", total_read);
                return 0;
            }
            int result = twz_error_errno(res.err);
            SYSTRACE("sys_msg_recv returning %d", result);
            return result;
        }

        total_read += res.val;

        // If we got 0 bytes, EOF was reached
        if (res.val == 0) break;
    }

    if (length) *length = total_read;
    SYSTRACE("sys_msg_recv returning 0 (%ld bytes)", total_read);
    // mlibc's contract is 0-or-errno, not a byte count.
    return 0;
}


int sys_getcwd(char *buf, size_t size) {
    SYSTRACE("sys_getcwd(buf=%p, size=%ld)", buf, size);
    if (!buf) {
        return EFAULT;
    }
    // There is no per-process cwd yet (sys_chdir is ENOSYS), so this is always the root.
    // A real implementation would track it via the NameRoot_Current nameroot.
    if (size < 2) {
        SYSTRACE("sys_getcwd returning ERANGE (size=%ld)", size);
        return ERANGE;
    }
    buf[0] = '/';
    buf[1] = '\0';
    SYSTRACE("sys_getcwd returning 0");
    return 0;
}

int sys_unlinkat(int dfd, const char *path, int flags) {
    SYSTRACE("sys_unlinkat(dfd=%d, path=%s, flags=%d)", dfd, path, flags);
    
    if (!path) {
        return EFAULT;
    }
    
    size_t path_len = strlen(path);
    
    // Twizzler's twz_rt_fd_remove doesn't support dirfd semantics the same way POSIX does.
    // For now, we require AT_FDCWD and handle the path as absolute/relative from root.
    if (dfd != AT_FDCWD && dfd >= 0) {
        // Would need to construct a path relative to dfd, which Twizzler doesn't directly support
        mlibc::sys_libc_log("sys_unlinkat: relative paths via dirfd not supported");
        return ENOTSUP;
    }
    
    twz_error err = twz_rt_fd_remove(path, path_len);
    int result = twz_error_errno(err);
    
    SYSTRACE("sys_unlinkat returning %d", result);
    return result;
}

int sys_sleep(time_t *secs, long *nanos) {
    SYSTRACE("sys_sleep(secs=%p, nanos=%p)", secs, nanos);
    
    if (!secs || !nanos) {
        return EFAULT;
    }
    
    // Create a duration from the input time
    struct duration dur = {
        .seconds = (uint64_t)(*secs),
        .nanos = (uint32_t)(*nanos)
    };
    
    // Sleep for the specified duration
    twz_rt_sleep(dur);
    
    // On Twizzler, we assume the sleep completes fully (no interrupts for now)
    // Return zero remaining time
    *secs = 0;
    *nanos = 0;
    
    SYSTRACE("sys_sleep returning 0");
    return 0;
}

int sys_isatty(int fd) {
    SYSTRACE("sys_isatty(fd=%d)", fd);
    
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        // Invalid file descriptor
        SYSTRACE("sys_isatty returning 0 (invalid fd)");
        return ENOTTY;
    }
    
    int result = (info.flags & FD_IS_TERMINAL) ? 0 : ENOTTY;
    
    SYSTRACE("sys_isatty returning %d", result);
    return result;
}

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/user.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <fcntl.h>
#include <pthread.h>

int sys_ioctl(int fd, unsigned long request, void *arg, int *result) {
    SYSTRACE("sys_ioctl(fd=%d, request=%lu, arg=%p, result=%p)", fd, request, arg, result);

    if (result) {
        *result = 0;
    }
    switch(request) {
        case TIOCGWINSZ:
            if (!arg) return EFAULT;
            return twz_error_errno(twz_rt_fd_get_config(fd, IO_REGISTER_WINSIZE, arg, sizeof(struct winsize)));
        case TIOCSWINSZ:
            if (!arg) return EFAULT;
            return twz_error_errno(twz_rt_fd_set_config(fd, IO_REGISTER_WINSIZE, arg, sizeof(struct winsize)));
        case FIONBIO: {
            if (!arg) return EFAULT;
            io_flags ioflags = 0;
            twz_error err = twz_rt_fd_get_config(fd, IO_REGISTER_IO_FLAGS, &ioflags, sizeof(ioflags));
            if (err != SUCCESS) return twz_error_errno(err);
            if (*(int *)arg) {
                ioflags |= IO_NONBLOCKING;
            } else {
                ioflags &= ~IO_NONBLOCKING;
            }
            return twz_error_errno(twz_rt_fd_set_config(fd, IO_REGISTER_IO_FLAGS, &ioflags, sizeof(ioflags)));
        }
        case TIOCGPGRP:
        case TIOCGSID:
            // Single process group / session, matching sys_getpgid and sys_getsid.
            if (!arg) return EFAULT;
            *(pid_t *)arg = 1;
            return 0;
        case TIOCSPGRP:
        case TIOCSCTTY:
            // Nothing to do with one process group and one terminal.
            return 0;
        default:
            // Reporting success for every unrecognized request means callers cannot tell that
            // their request was ignored. FIONREAD in particular has no ABI support today, so
            // callers need to see the failure and fall back.
            SYSTRACE("sys_ioctl returning ENOTTY (unhandled request %lu)", request);
            return ENOTTY;
    }
}

int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    SYSTRACE("sys_connect(sockfd=%d, addr=%p, addrlen=%d)", sockfd, addr, addrlen);
    
    if (!addr) return EFAULT;
    
    struct socket_address twz_addr = {};
    int err = sockaddr_to_twz_addr(addr, addrlen, twz_addr);
    if (err) {
        SYSTRACE("sys_connect returning %d", err);
        return err;
    }
    
    // POSIX connect() carries no protocol info, so use the type recorded at socket() time.
    enum prot_kind prot = socket_prot_get(sockfd);

    // Reconnect the socket to the new address
    struct socket_bind_info bind_info = {.addr = twz_addr, .prot = prot};
    twz_error result = twz_rt_fd_reopen(sockfd, OpenKind_SocketConnect, OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        &bind_info, sizeof(bind_info));

    int retval = twz_error_errno(result);
    if (retval == 0) {
        socket_flags_reapply(sockfd);
    }
    SYSTRACE("sys_connect returning %d", retval);
    return retval;
}

int sys_pselect(int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask, int *num_events) {
        SYSTRACE("sys_pselect(nfds=%d, readfds=%p, writefds=%p, exceptfds=%p, timeout=%p, sigmask=%p, num_events=%p)",
            nfds, readfds, writefds, exceptfds, timeout, sigmask, num_events);
    
    struct option_duration dur = {};
    if (timeout) {
        dur.dur.seconds = timeout->tv_sec;
        dur.dur.nanos = timeout->tv_nsec;
        dur.is_some = 1;
    } else {
        dur.is_some = 0;
    }

    (void)sigmask; // Twizzler doesn't have signal masks, so we ignore this parameter for now
    struct io_result res = twz_rt_fd_select(nfds, readfds, writefds, exceptfds, dur);

    if (res.err != SUCCESS) {
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_pselect returning %d", result);
        return result;
    }
        
    if (num_events) {
        *num_events = (int)res.val;
    }
	return 0;
}

int sys_ppoll(struct pollfd *fds, nfds_t count, const struct timespec *ts, const sigset_t *mask, int *num_events) {
    SYSTRACE("sys_ppoll(fds=%p, count=%ld, ts=%p, mask=%p, num_events=%p)", fds, count, ts, mask, num_events);
    
    struct option_duration dur = {};
    if (ts) {
        dur.dur.seconds = ts->tv_sec;
        dur.dur.nanos = ts->tv_nsec;
        dur.is_some = 1;
    } else {
        dur.is_some = 0;
    }

    (void)mask; // Twizzler doesn't have signal masks, so we ignore this parameter for now
    struct io_result res = twz_rt_fd_poll(fds, count, dur);

    if (res.err != SUCCESS) {
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_ppoll returning %d", result);
        return result;
    }
        
    if (num_events) {
        *num_events = (int)res.val;
    }
	return 0;
}

int sys_poll(struct pollfd *fds, nfds_t count, int timeout, int *num_events) {
    struct timespec ts = {};
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
    } else if (timeout == 0) {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    }
    if (timeout < 0) {
        return sys_ppoll(fds, count, nullptr, nullptr, num_events);
    }
    return sys_ppoll(fds, count, &ts, nullptr, num_events);
}


int sys_pipe(int *fds, int flags) {
    SYSTRACE("sys_pipe(fds=%p, flags=%d)", fds, flags);
    
    if (!fds) {
        return EFAULT;
    }
    
    // Open one pipe with both read and write access
    struct open_result pipe_result = twz_rt_fd_open(
        OpenKind_Pipe,
        OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        NULL,
        0
    );
    
    if (pipe_result.err != 0) {
        int result = twz_error_errno(pipe_result.err);
        SYSTRACE("sys_pipe returning %d (pipe open failed)", result);
        return result;
    }
    
    // Duplicate the pipe for the write end
    descriptor write_fd;
    twz_error dup_err = twz_rt_fd_cmd(pipe_result.fd, FD_CMD_DUP, NULL, &write_fd);
    
    if (dup_err != 0) {
        twz_rt_fd_close(pipe_result.fd);
        int result = twz_error_errno(dup_err);
        SYSTRACE("sys_pipe returning %d (dup failed)", result);
        return result;
    }
    
    // Shutdown the write side of the read end (bit 1 = 0b10 = 2)
    uint32_t shutdown_write = 2;  // FD_CMD_SHUTDOWN flag for write side
    twz_error shutdown_err = twz_rt_fd_cmd(pipe_result.fd, FD_CMD_SHUTDOWN, &shutdown_write, NULL);
    
    if (shutdown_err != 0) {
        twz_rt_fd_close(pipe_result.fd);
        twz_rt_fd_close(write_fd);
        int result = twz_error_errno(shutdown_err);
        SYSTRACE("sys_pipe returning %d (shutdown write side failed)", result);
        return result;
    }
    
    // Shutdown the read side of the write end (bit 0 = 0b01 = 1)
    uint32_t shutdown_read = 1;  // FD_CMD_SHUTDOWN flag for read side
    shutdown_err = twz_rt_fd_cmd(write_fd, FD_CMD_SHUTDOWN, &shutdown_read, NULL);
    
    if (shutdown_err != 0) {
        twz_rt_fd_close(pipe_result.fd);
        twz_rt_fd_close(write_fd);
        int result = twz_error_errno(shutdown_err);
        SYSTRACE("sys_pipe returning %d (shutdown read side failed)", result);
        return result;
    }
    
    // Store the file descriptors: [0] is read, [1] is write
    fds[0] = pipe_result.fd;
    fds[1] = write_fd;
    
    SYSTRACE("sys_pipe returning 0 (fds[0]=%d for read, fds[1]=%d for write)", fds[0], fds[1]);
    return 0;
}

int sys_fork(pid_t *child) {
    SYSTRACE("sys_fork(child=%p)", child);
	int result = ENOSYS;
	SYSTRACE("sys_fork returning %d", result);
	return result;
}

int sys_waitpid(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
    SYSTRACE("sys_waitpid(pid=%d, status=%p, flags=%d, ru=%p, ret_pid=%p)", pid, status, flags, ru, ret_pid);

    // In Twizzler, spawned process descriptors are used as pids (see sys_spawn).
    // We don't support wait(-1) or wait(0) yet.
    if (pid <= 0) {
        SYSTRACE("sys_waitpid returning ECHILD (pid=%d not supported)", pid);
        return ECHILD;
    }

    descriptor fd = (descriptor)pid;

    // Check current status first (handles WNOHANG and already-exited processes).
    uint64_t proc_status = 0;
    twz_error err = twz_rt_fd_get_config(fd, IO_REGISTER_STATUS, &proc_status, sizeof(proc_status));
    if (err != SUCCESS) {
        SYSTRACE("sys_waitpid returning ECHILD (fd_get_config failed: %d)", twz_error_errno(err));
        return ECHILD;
    }

    if (!(proc_status & STATUS_FLAG_TERMINATED)) {
        if (flags & WNOHANG) {
            // Not yet exited and caller doesn't want to block.
            if (ret_pid) *ret_pid = 0;
            SYSTRACE("sys_waitpid returning 0 (WNOHANG, not exited)");
            return 0;
        }

        // Block until the compartment exits: read() on a CompartmentFile blocks until
        // the compartment state changes (exits or changes), then we re-check status.
        char dummy;
        struct io_ctx ctx = {
            .flags = 0,
            .offset = FD_POS,
            .timeout = { .dur = {.seconds = 0, .nanos = 0}, .is_some = 0 },
        };
        do {
            twz_rt_fd_pread(fd, &dummy, 1, &ctx);

            err = twz_rt_fd_get_config(fd, IO_REGISTER_STATUS, &proc_status, sizeof(proc_status));
            if (err != SUCCESS) {
                SYSTRACE("sys_waitpid returning ECHILD (fd_get_config failed after wait: %d)", twz_error_errno(err));
                return ECHILD;
            }
        } while (!(proc_status & STATUS_FLAG_TERMINATED));
    }

    // Process has exited. Extract exit code from lower 32 bits of status word.
    int exit_code = (int)(proc_status & 0xffffffffu);

    // Encode in POSIX wait status format: normal exit with code in bits [15:8].
    if (status)
        *status = (exit_code & 0xff) << 8;
    if (ret_pid)
        *ret_pid = pid;

    SYSTRACE("sys_waitpid returning 0, pid=%d exit_code=%d", pid, exit_code);
    return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    SYSTRACE("sys_execve(path=%s, argv=%p, envp=%p)", path, argv, envp);
    struct exec_spawn_args args = {
        .prog = path,
        .args = argv,
        .env = envp,
        .fd_binds = NULL,
        .fd_bind_count = 0,
        .flags = 0,
    };
	struct open_result res = twz_rt_exec_spawn(&args);
    if (res.err != SUCCESS) {
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_execve returning %d", result);
        return result;
    }
	SYSTRACE("sys_execve exiting");
    sys_exit(0);
    return 0;
}


int sys_sigprocmask(int how, const sigset_t *set, sigset_t *old) {
    SYSTRACE("sys_sigprocmask(how=%d, set=%p, old=%p)", how, set, old);
	if(old)
		sigSetMask(old, __atomic_load_n(&sigBlocked, __ATOMIC_ACQUIRE));
	if(!set)
		return 0;

	// SIGKILL and SIGSTOP can never be blocked.
	unsigned long requested = sigMaskOf(set) & ~(sigBit(SIGKILL) | sigBit(SIGSTOP));
	switch(how) {
		case SIG_BLOCK:
			__atomic_fetch_or(&sigBlocked, requested, __ATOMIC_ACQ_REL);
			break;
		case SIG_UNBLOCK:
			__atomic_fetch_and(&sigBlocked, ~requested, __ATOMIC_ACQ_REL);
			break;
		case SIG_SETMASK:
			__atomic_store_n(&sigBlocked, requested, __ATOMIC_RELEASE);
			break;
		default:
			return EINVAL;
	}
	// Unblocking can make queued signals deliverable.
	sigDrainPending();
	return 0;
}

int sys_thread_sigmask(int how, const sigset_t *set, sigset_t *retrieve) {
	// The mask is compartment-wide rather than per-thread: signals are only ever delivered to the
	// compartment's main thread, so a per-thread mask would have nothing to gate.
	return sys_sigprocmask(how, set, retrieve);
}

int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid) {
    SYSTRACE("sys_setresuid(ruid=%d, euid=%d, suid=%d)", ruid, euid, suid);
	int result = ENOSYS;
	SYSTRACE("sys_setresuid returning %d", result);
	return result;
}

int sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
    SYSTRACE("sys_setresgid(rgid=%d, egid=%d, sgid=%d)", rgid, egid, sgid);
	int result = ENOSYS;
	SYSTRACE("sys_setresgid returning %d", result);
	return result;
}

// There is one identity and no user database, so report it rather than refusing. The matching
// setres*/setre* calls stay ENOSYS: succeeding there would tell a caller it had changed
// privileges when it had not.
int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
    SYSTRACE("sys_getresuid(ruid=%p, euid=%p, suid=%p)", ruid, euid, suid);
	if (ruid)
		*ruid = 0;
	if (euid)
		*euid = 0;
	if (suid)
		*suid = 0;
	return 0;
}

int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid) {
    SYSTRACE("sys_getresgid(rgid=%p, egid=%p, sgid=%p)", rgid, egid, sgid);
	if (rgid)
		*rgid = 0;
	if (egid)
		*egid = 0;
	if (sgid)
		*sgid = 0;
	return 0;
}

int sys_setreuid(uid_t ruid, uid_t euid) {
    SYSTRACE("sys_setreuid(ruid=%d, euid=%d)", ruid, euid);
	int result = ENOSYS;
	SYSTRACE("sys_setreuid returning %d", result);
	return result;
}

int sys_setregid(gid_t rgid, gid_t egid) {
    SYSTRACE("sys_setregid(rgid=%d, egid=%d)", rgid, egid);
	int result = ENOSYS;
	SYSTRACE("sys_setregid returning %d", result);
	return result;
}

int sys_sysinfo(struct sysinfo *info) {
    SYSTRACE("sys_sysinfo(info=%p)", info);
    if (!info)
        return EFAULT;
    struct system_info si = twz_rt_get_sysinfo();
    struct duration uptime = twz_rt_get_monotonic_time();
    memset(info, 0, sizeof(*info));
    info->uptime = (long)uptime.seconds;
    info->procs = (unsigned short)si.available_parallelism;
    info->mem_unit = (unsigned int)si.page_size;
    SYSTRACE("sys_sysinfo returning 0");
    return 0;
}

void sys_yield() {
    SYSTRACE("sys_yield()");
	twz_rt_yield_now();
}

extern "C" void __mlibc_enter_thread(void *arg);
int sys_clone(void **tcb, pid_t *pid_out, void *entry, void *user_arg, bool returns_int) {
    SYSTRACE("sys_clone(tcb=*%p, pid_out=%p, user_arg=%p)", tcb, pid_out, user_arg);

	// Allocate a twz_thread_args and fill it out
	auto args = (twz_thread_args*)getAllocator().allocate(sizeof(twz_thread_args));
    SYSTRACE("sys_clone: allocated thread args at %p, with entry %p and user_arg %p", args, entry, user_arg);
	if (!args)
		return ENOMEM;
	
	args->entry = entry;
	args->user_arg = user_arg;
	args->tcb = reinterpret_cast<Tcb *>(tcb);
	args->returns_int = returns_int;
	args->is_joinable = true;
	
	// Use Twizzler's thread spawning API
	struct spawn_args spawn_args_val = {
		.stack_size = 0x200000,  // Default 2MB stack; runtime will allocate
		.start = reinterpret_cast<uintptr_t>(&__mlibc_enter_thread),
		.arg = reinterpret_cast<uintptr_t>(args),
	};
	
	struct spawn_result result = twz_rt_spawn_thread(spawn_args_val);
	
	if (result.err != SUCCESS) {
		int errno_val = twz_error_errno(result.err);
		getAllocator().free(args);
		SYSTRACE("sys_clone returning %d", errno_val);
		return errno_val;
	}
	
	*pid_out = result.id;
	*tcb = result.tcb;
	SYSTRACE("sys_clone returning 0, thread_id=%d", result.id);
	return 0;
}

int sys_spawn(int *pid, const char *path, char *const*argv, char *const*envp) {
	SYSTRACE("sys_spawn(pid=%p, path=(%p)%s, argv=%p, envp=%p)", pid, path, path, argv, envp);
	
	if (!pid || !path || !argv || !envp) {
		SYSTRACE("sys_spawn returning EINVAL (null argument)");
		return EINVAL;
	}
	
	// Set up exec_spawn_args for Twizzler's exec API
	struct exec_spawn_args spawn_args = {
		.prog = path,
		.args = (const char * const *)argv,
		.env = (const char * const *)envp,
		.fd_binds = nullptr,      // For now, inherit file descriptors from parent
		.fd_bind_count = 0,
		.flags = 0,
	};
	
	struct open_result result = twz_rt_exec_spawn(&spawn_args);
	
	if (result.err != SUCCESS) {
		int errno_val = twz_error_errno(result.err);
		SYSTRACE("sys_spawn returning %d", errno_val);
		return errno_val;
	}
	
	// The returned fd is the process descriptor/handle
	// Store it as the "PID" (in Twizzler, this might be a handle rather than a traditional PID)
	*pid = result.fd;
	
	SYSTRACE("sys_spawn returning 0, spawned process fd=%d", result.fd);
	return 0;
}

extern "C" const char __mlibc_syscall_begin[1];
extern "C" const char __mlibc_syscall_end[1];

int sys_tgkill(int tgid, int tid, int sig) {
    SYSTRACE("sys_tgkill(tgid=%d, tid=%d, sig=%d)", tgid, tid, sig);
	// Delivery is compartment-wide, so there is no thread to select: pthread_kill() against any
	// thread of this process raises the signal against the process.
	(void)tid;
	int result = sys_kill(tgid, sig);
	SYSTRACE("sys_tgkill returning %d", result);
	return result;
}

int sys_tcgetattr(int fd, struct termios *attr) {
    SYSTRACE("sys_tcgetattr(fd=%d, attr=%p)", fd, attr);
    int result = twz_error_errno(twz_rt_fd_get_config(fd, IO_REGISTER_TERMIOS, attr, sizeof(*attr)));
    SYSTRACE("sys_tcgetattr returning %d", result);
    return result;
}

int sys_tcsetattr(int fd, int optional_action, const struct termios *attr) {
    SYSTRACE("sys_tcsetattr(fd=%d, optional_action=%d, attr=%p)", fd, optional_action, attr);
    int result = twz_error_errno(twz_rt_fd_set_config(fd, IO_REGISTER_TERMIOS, attr, sizeof(*attr)));
    SYSTRACE("sys_tcsetattr returning %d", result);
    return result;
}

int sys_tcflush(int fd, int queue) {
    SYSTRACE("sys_tcflush(fd=%d, queue=%d)", fd, queue);
	int result = 0;
	SYSTRACE("sys_tcflush returning %d", result);
	return result;
}

int sys_tcdrain(int fd) {
    SYSTRACE("sys_tcdrain(fd=%d)", fd);
	int result = 0;
	SYSTRACE("sys_tcdrain returning %d", result);
	return result;
}

int sys_tcflow(int fd, int action) {
    SYSTRACE("sys_tcflow(fd=%d, action=%d)", fd, action);
	int result = 0;
	SYSTRACE("sys_tcflow returning %d", result);
	return result;
}

int sys_access(const char *path, int mode) {
    SYSTRACE("sys_access(path=%s, mode=%d)", path, mode);
    int fd;
    int e = sys_open(path, O_RDONLY, 0, &fd);
    if (e != 0) {
        SYSTRACE("sys_access returning %d (open failed)", e);
        return e;
    }
    twz_rt_fd_close(fd);
    SYSTRACE("sys_access returning 0");
	return 0;
}

int sys_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    SYSTRACE("sys_faccessat(dirfd=%d, pathname=%s, mode=%d, flags=%d)", dirfd, pathname, mode, flags);

    int fd = -1;
    // sys_openat reports a positive errno, so this must not test for e < 0.
    int e = sys_openat(dirfd, pathname, O_RDONLY, 0, &fd);
    if (e != 0) {
        SYSTRACE("sys_faccessat returning %d (open failed)", e);
        return e;
    }
    twz_rt_fd_close(fd);
    SYSTRACE("sys_faccessat returning 0");
	return 0;
}

int sys_accept(int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags) {
    SYSTRACE("sys_accept(fd=%d, newfd=%p, addr_ptr=%p, addr_length=%p, flags=%d)", fd, newfd, addr_ptr, addr_length, flags);
    
    if (!newfd) return EFAULT;
    
    // Open a new socket descriptor for the accepted connection
    // Must pass the listening socket fd as bind data
    descriptor listen_fd = fd;
    struct open_result res = twz_rt_fd_open(OpenKind_SocketAccept, OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        &listen_fd, sizeof(listen_fd));
    
    if (res.err != SUCCESS) {
        int result = twz_error_errno(res.err);
        SYSTRACE("sys_accept returning %d", result);
        return result;
    }
    
    *newfd = res.fd;

    // An accepted connection is always a stream, whatever the listener was recorded as.
    socket_prot_set(res.fd, SOCK_STREAM);

    // If address buffer provided, fill it with peer address
    if (addr_ptr && addr_length && *addr_length > 0) {
        struct socket_address peer_addr = {};
        twz_error err = twz_rt_fd_get_config(*newfd, IO_REGISTER_PEER, &peer_addr, sizeof(peer_addr));
        
        if (err == SUCCESS) {
            socklen_t needed_len = 0;
            if (peer_addr.kind == AddrKind_Ipv4) {
                needed_len = sizeof(struct sockaddr_in);
                if (*addr_length >= needed_len) {
                    struct sockaddr_in *sin = (struct sockaddr_in *)addr_ptr;
                    sin->sin_family = AF_INET;
                    sin->sin_port = htons(peer_addr.port);
                    memcpy(&sin->sin_addr, peer_addr.addr_octets.v4, 4);
                }
            } else if (peer_addr.kind == AddrKind_Ipv6) {
                needed_len = sizeof(struct sockaddr_in6);
                if (*addr_length >= needed_len) {
                    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr_ptr;
                    sin6->sin6_family = AF_INET6;
                    sin6->sin6_port = htons(peer_addr.port);
                    memcpy(&sin6->sin6_addr, peer_addr.addr_octets.v6, 16);
                    sin6->sin6_flowinfo = peer_addr.flowinfo;
                    sin6->sin6_scope_id = peer_addr.scope_id;
                }
            }
            *addr_length = needed_len;
        }
    }
    
    SYSTRACE("sys_accept returning 0, newfd=%d", *newfd);
    return 0;
}

int sys_bind(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
    SYSTRACE("sys_bind(fd=%d, addr_ptr=%p, addr_length=%d)", fd, addr_ptr, addr_length);
    
    if (!addr_ptr) return EFAULT;
    
    struct socket_address twz_addr = {};
    int err = sockaddr_to_twz_addr(addr_ptr, addr_length, twz_addr);
    if (err) {
        SYSTRACE("sys_bind returning %d", err);
        return err;
    }
    
    // POSIX bind() carries no protocol info, so use the type recorded at socket() time.
    enum prot_kind prot = socket_prot_get(fd);

    // Reopen the socket to bind to the specified address
    struct socket_bind_info bind_info = {.addr = twz_addr, .prot = prot};
    twz_error result = twz_rt_fd_reopen(fd, OpenKind_SocketBind, OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        &bind_info, sizeof(bind_info));

    int retval = twz_error_errno(result);
    if (retval == 0) {
        socket_flags_reapply(fd);
    }
    SYSTRACE("sys_bind returning %d", retval);
    return retval;
}

// Set one bit of the socket flags word. All the flags (NODELAY, ONLYV6, BROADCAST, ...) share a
// single register, so this works from the shadow copy rather than overwriting the whole word --
// see the comment on socket_flags_table for why it cannot be read back from the runtime.
static int socket_flag_set(int fd, uint32_t bit, bool on) {
    uint32_t flags = socket_flags_shadow(fd);
    if (on) {
        flags |= bit;
    } else {
        flags &= ~bit;
    }
    twz_error err = twz_rt_fd_set_config(fd, IO_REGISTER_SOCKET_FLAGS, &flags, sizeof(flags));
    if (err != SUCCESS) {
        return twz_error_errno(err);
    }
    socket_flags_shadow_store(fd, flags);
    return 0;
}

static int socket_flag_get(int fd, uint32_t bit, void *buffer, socklen_t *size) {
    if (!buffer || !size || *size < sizeof(int)) return EINVAL;
    *(int *)buffer = (socket_flags_shadow(fd) & bit) ? 1 : 0;
    *size = sizeof(int);
    return 0;
}

// A timeout register the runtime does not implement for sockets yet. Reporting ENOPROTOOPT is
// the accurate answer -- silently accepting would leave callers believing a blocking read will
// time out when it never will.
static int socket_set_timeout(int fd, uint32_t reg, const void *buffer, socklen_t size) {
    if (size < sizeof(struct timeval)) return EINVAL;
    const struct timeval *tv = (const struct timeval *)buffer;
    struct option_duration dur = {
        .dur = {.seconds = (uint64_t)tv->tv_sec, .nanos = (uint32_t)(tv->tv_usec * 1000)},
        .is_some = 1
    };
    twz_error err = twz_rt_fd_set_config(fd, reg, &dur, sizeof(dur));
    if (err != SUCCESS) {
        SYSTRACE("sys_setsockopt: timeout register %u unsupported by the runtime", reg);
        return ENOPROTOOPT;
    }
    return 0;
}

int sys_setsockopt(int fd, int layer, int number, const void *buffer, socklen_t size) {
    SYSTRACE("sys_setsockopt(fd=%d, layer=%d, number=%d, buffer=%p, size=%d)", fd, layer, number, buffer, size);

    if (!buffer) return EFAULT;

    // Only IO_REGISTER_SOCKET_FLAGS is implemented for sockets in the runtime, so options map
    // either onto a flag bit, onto nothing (accepted below), or onto ENOPROTOOPT.
    if (layer == SOL_SOCKET) {
        switch(number) {
            case SO_RCVTIMEO:
                return socket_set_timeout(fd, IO_REGISTER_READTIMEOUT, buffer, size);
            case SO_SNDTIMEO:
                return socket_set_timeout(fd, IO_REGISTER_WRITETIMEOUT, buffer, size);
            case SO_BROADCAST:
                if (size < sizeof(int)) return EINVAL;
                return socket_flag_set(fd, SOCKET_FLAGS_BROADCAST, *(const int *)buffer != 0);
            // Accepted without effect: ignoring these changes performance or address-reuse
            // policy, not the observable correctness of a connection, and rejecting them
            // makes ordinary server code fail at startup.
            case SO_REUSEADDR:
            case SO_REUSEPORT:
            case SO_KEEPALIVE:
            case SO_SNDBUF:
            case SO_RCVBUF:
            case SO_LINGER:
            case SO_DONTROUTE:
                SYSTRACE("sys_setsockopt: accepting SOL_SOCKET option %d without effect", number);
                return 0;
            default:
                SYSTRACE("sys_setsockopt returning ENOPROTOOPT (SOL_SOCKET option %d)", number);
                return ENOPROTOOPT;
        }
    } else if (layer == IPPROTO_TCP) {
        switch(number) {
            case TCP_NODELAY:
                if (size < sizeof(int)) return EINVAL;
                return socket_flag_set(fd, SOCKET_FLAGS_NODELAY, *(const int *)buffer != 0);
            case TCP_KEEPIDLE:
            case TCP_KEEPINTVL:
            case TCP_KEEPCNT:
                return 0;
            default:
                SYSTRACE("sys_setsockopt returning ENOPROTOOPT (TCP option %d)", number);
                return ENOPROTOOPT;
        }
    } else if (layer == IPPROTO_IP) {
        switch(number) {
            case IP_MULTICAST_LOOP:
                if (size < sizeof(int)) return EINVAL;
                return socket_flag_set(fd, SOCKET_FLAGS_MULTICAST_LOOP_V4, *(const int *)buffer != 0);
            default:
                // IP_TTL, IP_MULTICAST_TTL and group membership have IO_REGISTER_* constants
                // reserved but no runtime implementation.
                SYSTRACE("sys_setsockopt returning ENOPROTOOPT (IP option %d)", number);
                return ENOPROTOOPT;
        }
    } else if (layer == IPPROTO_IPV6) {
        switch(number) {
            case IPV6_V6ONLY:
                if (size < sizeof(int)) return EINVAL;
                return socket_flag_set(fd, SOCKET_FLAGS_ONLYV6, *(const int *)buffer != 0);
            case IPV6_MULTICAST_LOOP:
                if (size < sizeof(int)) return EINVAL;
                return socket_flag_set(fd, SOCKET_FLAGS_MULTICAST_LOOP_V6, *(const int *)buffer != 0);
            default:
                SYSTRACE("sys_setsockopt returning ENOPROTOOPT (IPV6 option %d)", number);
                return ENOPROTOOPT;
        }
    }

    SYSTRACE("sys_setsockopt returning ENOPROTOOPT (unsupported level %d)", layer);
    return ENOPROTOOPT;
}

int sys_sockname(int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length,
		socklen_t *actual_length) {
    SYSTRACE("sys_sockname(fd=%d, addr_ptr=%p, max_addr_length=%d, actual_length=%p)", fd, addr_ptr, max_addr_length, actual_length);
    
    if (!addr_ptr || !actual_length) return EFAULT;
    
    // Query socket local address
    struct socket_address sock_addr = {};
    twz_error err = twz_rt_fd_get_config(fd, IO_REGISTER_ADDR, &sock_addr, sizeof(sock_addr));
    
    if (err != SUCCESS) {
        int result = twz_error_errno(err);
        SYSTRACE("sys_sockname returning %d", result);
        return result;
    }
    
    // Convert Twizzler socket_address to POSIX sockaddr
    socklen_t needed_len = 0;
    if (sock_addr.kind == AddrKind_Ipv4) {
        needed_len = sizeof(struct sockaddr_in);
        if (max_addr_length >= needed_len) {
            struct sockaddr_in *sin = (struct sockaddr_in *)addr_ptr;
            sin->sin_family = AF_INET;
            sin->sin_port = htons(sock_addr.port);
            memcpy(&sin->sin_addr, sock_addr.addr_octets.v4, 4);
        }
    } else if (sock_addr.kind == AddrKind_Ipv6) {
        needed_len = sizeof(struct sockaddr_in6);
        if (max_addr_length >= needed_len) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr_ptr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(sock_addr.port);
            memcpy(&sin6->sin6_addr, sock_addr.addr_octets.v6, 16);
            sin6->sin6_flowinfo = sock_addr.flowinfo;
            sin6->sin6_scope_id = sock_addr.scope_id;
        }
    }
    
    *actual_length = needed_len;
    SYSTRACE("sys_sockname returning 0");
    return 0;
}

int sys_peername(int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length,
		socklen_t *actual_length) {
    SYSTRACE("sys_peername(fd=%d, addr_ptr=%p, max_addr_length=%d, actual_length=%p)", fd, addr_ptr, max_addr_length, actual_length);
    
    if (!addr_ptr || !actual_length) return EFAULT;
    
    // Query socket peer address
    struct socket_address peer_addr = {};
    twz_error err = twz_rt_fd_get_config(fd, IO_REGISTER_PEER, &peer_addr, sizeof(peer_addr));
    
    if (err != SUCCESS) {
        int result = twz_error_errno(err);
        SYSTRACE("sys_peername returning %d", result);
        return result;
    }
    
    // Convert Twizzler socket_address to POSIX sockaddr
    socklen_t needed_len = 0;
    if (peer_addr.kind == AddrKind_Ipv4) {
        needed_len = sizeof(struct sockaddr_in);
        if (max_addr_length >= needed_len) {
            struct sockaddr_in *sin = (struct sockaddr_in *)addr_ptr;
            sin->sin_family = AF_INET;
            sin->sin_port = htons(peer_addr.port);
            memcpy(&sin->sin_addr, peer_addr.addr_octets.v4, 4);
        }
    } else if (peer_addr.kind == AddrKind_Ipv6) {
        needed_len = sizeof(struct sockaddr_in6);
        if (max_addr_length >= needed_len) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)addr_ptr;
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(peer_addr.port);
            memcpy(&sin6->sin6_addr, peer_addr.addr_octets.v6, 16);
            sin6->sin6_flowinfo = peer_addr.flowinfo;
            sin6->sin6_scope_id = peer_addr.scope_id;
        }
    }
    
    *actual_length = needed_len;
    SYSTRACE("sys_peername returning 0");
    return 0;
}

int sys_listen(int fd, int backlog) {
    SYSTRACE("sys_listen(fd=%d, backlog=%d)", fd, backlog);
    
    if (backlog < 0) return EINVAL;
    
    // In Twizzler, listening might be implicit upon bind, but we could use set_config
    // For now, just report success as the socket is ready to accept connections
    SYSTRACE("sys_listen returning 0");
    return 0;
}

int sys_shutdown(int sockfd, int how) {
    SYSTRACE("sys_shutdown(sockfd=%d, how=%d)", sockfd, how);
    
    if (how < SHUT_RD || how > SHUT_RDWR) return EINVAL;

    // FD_CMD_SHUTDOWN takes a bitmask: bit 0 is the read side, bit 1 the write side. POSIX
    // SHUT_* are consecutive values starting at 0, so they must be translated -- passing `how`
    // straight through made SHUT_RD a no-op request (rejected), SHUT_WR shut down the *read*
    // side, and SHUT_RDWR shut down only the write side.
    uint32_t shutdown_flags;
    switch (how) {
        case SHUT_RD:   shutdown_flags = 1; break;
        case SHUT_WR:   shutdown_flags = 2; break;
        default:        shutdown_flags = 3; break;  // SHUT_RDWR
    }
    twz_error err = twz_rt_fd_cmd(sockfd, FD_CMD_SHUTDOWN, &shutdown_flags, NULL);
    
    int result = twz_error_errno(err);
    SYSTRACE("sys_shutdown returning %d", result);
    return result;
}

int sys_getpriority(int which, id_t who, int *value) {
    SYSTRACE("sys_getpriority(which=%d, who=%d, value=%p)", which, who, value);
	int result = ENOSYS;
	SYSTRACE("sys_getpriority returning %d", result);
	return result;
}

int sys_setpriority(int which, id_t who, int prio) {
    SYSTRACE("sys_setpriority(which=%d, who=%d, prio=%d)", which, who, prio);
	int result = ENOSYS;
	SYSTRACE("sys_setpriority returning %d", result);
	return result;
}

int sys_open_dir(const char *path, int *fd) {
	return sys_open(path, O_RDONLY | O_DIRECTORY, 0, fd);
}

#define ALIGN_UP(value, alignment) (((value) + (alignment) - 1) & ~((alignment) - 1))
int sys_read_entries(int handle, void *buffer, size_t max_size, size_t *bytes_read) {
	SYSTRACE("sys_read_entries initial bytes_read: %ld", *bytes_read);
	*bytes_read = 0;
	off_t off = lseek(handle, 0, SEEK_CUR);
	if(off == -1){
		return errno;
	}
	size_t dirent_size = offsetof(struct dirent, d_name);
	// struct name_entry is ~350 bytes, so sizing this batch off the caller's buffer would put
	// an order of magnitude more than max_size on the stack. Cap it and let the caller loop.
	static const size_t max_batch = 16;
	size_t nr_twz_entries = max_size / dirent_size;
	if(nr_twz_entries > max_batch)
		nr_twz_entries = max_batch;
	if(nr_twz_entries == 0)
		nr_twz_entries = 1;
	SYSTRACE("sys_read_entries(handle=%d, buffer=%p, max_size=%ld, bytes_read=%p), off=%ld, nr_twz_entries=%ld",
		handle, buffer, max_size, bytes_read, off, nr_twz_entries);

	struct name_entry twznames[max_batch];
	struct io_result res = twz_rt_fd_enumerate_names(handle, twznames, nr_twz_entries, off / sizeof(struct name_entry));
	if(res.err != 0)
		return twz_error_errno(res.err);

	SYSTRACE("twz_rt_fd_enumerate_names returned %ld entries", res.val);
	if(res.val == 0)
		return 0;

	// it's null terminated
	if(twznames[0].name_len + 1 + dirent_size > max_size)
		return EINVAL;

    size_t count = 0;
	for(size_t i = 0;i < res.val;i++) {
		struct name_entry *entry = &twznames[i];
		if(entry->name_len + 1 + dirent_size + *bytes_read > max_size)
			break;

		size_t thislen = dirent_size + 1 + entry->name_len;
		size_t this_reclen = ALIGN_UP(thislen, 8);
		struct dirent *target = (struct dirent *)((char *)buffer + *bytes_read);
		target->d_ino = objid_to_ino(entry->info.id);
		target->d_reclen = this_reclen;
		SYSTRACE("entry %ld: name=%.*s, ino=%ld, reclen=%hu, namelen=%d, direntsz = %ld, direntaln = %ld, thislen=%d", i, entry->name_len, entry->name, target->d_ino, target->d_reclen, entry->name_len, dirent_size, thislen, 8);

		char type = DT_UNKNOWN;
		switch(entry->info.kind) {
			case FdKind_Regular:
				type = DT_REG;
				break;
			case FdKind_Directory:
				type = DT_DIR;
				break;
			case FdKind_SymLink:
				type = DT_LNK;
				break;
			default: break;
		}

		target->d_type = type;
		target->d_off = this_reclen + *bytes_read;
		memcpy(target->d_name, entry->name, entry->name_len);
		target->d_name[entry->name_len] = 0;

		*bytes_read += this_reclen;
        count += 1;
	}
	SYSTRACE("total bytes read: %ld (%ld entries)", *bytes_read, count);

	lseek(handle, count * sizeof(struct name_entry) + off, SEEK_SET);
	return 0;
}

int sys_uname(struct utsname *buf) {
    SYSTRACE("sys_uname(buf=%p)", buf);
	
	if (!buf) {
		int result = EFAULT;
		SYSTRACE("sys_uname returning %d", result);
		return result;
	}

	// Fill in the utsname structure with Twizzler information
	snprintf(buf->sysname, sizeof(buf->sysname), "Twizzler");
	snprintf(buf->nodename, sizeof(buf->nodename), "twizzler");
	snprintf(buf->release, sizeof(buf->release), "1.0");
	snprintf(buf->version, sizeof(buf->version), "twizzler-runtime");
#if defined(__aarch64__)
	snprintf(buf->machine, sizeof(buf->machine), "aarch64");
#elif defined(__x86_64__)
	snprintf(buf->machine, sizeof(buf->machine), "x86_64");
#else
	snprintf(buf->machine, sizeof(buf->machine), "unknown");
#endif

	int result = 0;
	SYSTRACE("sys_uname returning %d", result);
	return result;
}

int sys_gethostname(char *buf, size_t bufsize) {
    SYSTRACE("sys_gethostname(buf=%p, bufsize=%ld)", buf, bufsize);
    strncpy(buf, "twizzler", bufsize);
	int result = 0;
	SYSTRACE("sys_gethostname returning %d", result);
	return result;
}

int sys_pread(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read) {
    SYSTRACE("sys_pread(fd=%d, buf=%p, n=%ld, off=%ld, bytes_read=%p)", fd, buf, n, off, bytes_read);
   	struct io_ctx ctx = {
		.flags = 0,
		.offset = off,
		.timeout = NO_DURATION,
	};
	if (bytes_read != nullptr) {
		*bytes_read = 0;
	}
	struct io_result res = twz_rt_fd_pread((descriptor)fd, buf, n, &ctx);
	if (res.err == SUCCESS) {
		if (bytes_read != nullptr) {
			*bytes_read = (ssize_t)res.val;
		}
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_pwrite(int fd, const void *buf, size_t n, off_t off, ssize_t *bytes_written) {
    SYSTRACE("pwrite(%d, %p, %ld, %ld)", fd, buf, n, off);

   	struct io_ctx ctx = {
		.flags = 0,
		.offset = off,
		.timeout = NO_DURATION,
	};
	if (bytes_written != nullptr) {
		*bytes_written = 0;
	}
	struct io_result res = twz_rt_fd_pwrite((descriptor)fd, buf, n, &ctx);
	if (res.err == SUCCESS) {
		if (bytes_written != nullptr) {
			*bytes_written = (ssize_t)res.val;
		}
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_getsockopt(int fd, int layer, int number, void *__restrict buffer, socklen_t *__restrict size) {
    SYSTRACE("sys_getsockopt(fd=%d, layer=%d, number=%d, buffer=%p, size=%p)", fd, layer, number, buffer, size);
    if (!buffer || !size) return EFAULT;

    if (layer == SOL_SOCKET) {
        switch(number) {
            case SO_ERROR:
                // No pending-error register exists; a connected socket reports success.
                if (*size < sizeof(int)) return EINVAL;
                *(int *)buffer = 0;
                *size = sizeof(int);
                SYSTRACE("sys_getsockopt returning 0, SO_ERROR=0");
                return 0;
            case SO_TYPE:
                if (*size < sizeof(int)) return EINVAL;
                *(int *)buffer = socket_type_get(fd);
                *size = sizeof(int);
                SYSTRACE("sys_getsockopt returning 0, SO_TYPE=%d", *(int *)buffer);
                return 0;
            case SO_BROADCAST:
                return socket_flag_get(fd, SOCKET_FLAGS_BROADCAST, buffer, size);
            // Reported as off, matching the fact that setting them has no effect.
            case SO_REUSEADDR:
            case SO_REUSEPORT:
            case SO_KEEPALIVE:
            case SO_DONTROUTE:
            case SO_OOBINLINE:
            case SO_ACCEPTCONN:
                if (*size < sizeof(int)) return EINVAL;
                *(int *)buffer = 0;
                *size = sizeof(int);
                return 0;
            default:
                SYSTRACE("sys_getsockopt returning ENOPROTOOPT (SOL_SOCKET option %d)", number);
                return ENOPROTOOPT;
        }
    } else if (layer == IPPROTO_TCP) {
        switch(number) {
            case TCP_NODELAY:
                return socket_flag_get(fd, SOCKET_FLAGS_NODELAY, buffer, size);
            default:
                SYSTRACE("sys_getsockopt returning ENOPROTOOPT (TCP option %d)", number);
                return ENOPROTOOPT;
        }
    } else if (layer == IPPROTO_IP) {
        switch(number) {
            case IP_MULTICAST_LOOP:
                return socket_flag_get(fd, SOCKET_FLAGS_MULTICAST_LOOP_V4, buffer, size);
            default:
                SYSTRACE("sys_getsockopt returning ENOPROTOOPT (IP option %d)", number);
                return ENOPROTOOPT;
        }
    } else if (layer == IPPROTO_IPV6) {
        switch(number) {
            case IPV6_V6ONLY:
                return socket_flag_get(fd, SOCKET_FLAGS_ONLYV6, buffer, size);
            case IPV6_MULTICAST_LOOP:
                return socket_flag_get(fd, SOCKET_FLAGS_MULTICAST_LOOP_V6, buffer, size);
            default:
                SYSTRACE("sys_getsockopt returning ENOPROTOOPT (IPV6 option %d)", number);
                return ENOPROTOOPT;
        }
    }

    SYSTRACE("sys_getsockopt returning ENOPROTOOPT (unsupported level %d)", layer);
    return ENOPROTOOPT;
}

int sys_sysconf(int num, long *ret) {
    SYSTRACE("sys_sysconf(num=%d, ret=%p)", num, ret);
    if (!ret)
        return EFAULT;
    struct system_info info = twz_rt_get_sysinfo();
	switch(num) {
    	case _SC_NPROCESSORS_CONF:
    	case _SC_NPROCESSORS_ONLN:
            *ret = info.available_parallelism;
            break;
        case _SC_PAGESIZE:
            *ret = info.page_size;
            break;
        // Limits. These mirror the values reported elsewhere in this file, so callers that
        // size buffers from sysconf agree with what the rest of the libc enforces.
        case _SC_OPEN_MAX:
            *ret = 1024;                    // matches sys_getrlimit's RLIMIT_NOFILE
            break;
        case _SC_CLK_TCK:
            *ret = TWZ_CLK_TCK;             // matches sys_times
            break;
        case _SC_THREAD_STACK_MIN:
            *ret = 0x200000;                // matches sys_prepare_stack's default
            break;
        case _SC_ARG_MAX:
            *ret = 2097152;
            break;
        case _SC_CHILD_MAX:
        case _SC_THREAD_THREADS_MAX:
        case _SC_STREAM_MAX:
            *ret = -1;                      // indeterminate
            break;
        case _SC_HOST_NAME_MAX:
            *ret = HOST_NAME_MAX;
            break;
        case _SC_LOGIN_NAME_MAX:
            *ret = LOGIN_NAME_MAX;
            break;
        case _SC_TTY_NAME_MAX:
            // No TTY_NAME_MAX is defined for this ABI; sys_ttyname reports "/dev/tty".
            *ret = _POSIX_TTY_NAME_MAX;
            break;
        case _SC_SYMLOOP_MAX:
            // Matches naming_core's MAX_SYMLINK_DEREF ceiling closely enough; no macro exists.
            *ret = _POSIX_SYMLOOP_MAX;
            break;
        case _SC_LINE_MAX:
            *ret = 2048;
            break;
        case _SC_IOV_MAX:
            *ret = IOV_MAX;
            break;
        case _SC_NGROUPS_MAX:
            *ret = 0;                       // sys_getgroups is unimplemented
            break;
        case _SC_ATEXIT_MAX:
            *ret = 32;
            break;
        case _SC_GETPW_R_SIZE_MAX:
        case _SC_GETGR_R_SIZE_MAX:
            *ret = 1024;
            break;
        case _SC_THREAD_KEYS_MAX:
            *ret = PTHREAD_KEYS_MAX;
            break;
        case _SC_THREAD_DESTRUCTOR_ITERATIONS:
            *ret = PTHREAD_DESTRUCTOR_ITERATIONS;
            break;
        // Memory. There is no per-compartment memory accounting exposed, so report unknown
        // rather than a fabricated size.
        case _SC_PHYS_PAGES:
        case _SC_AVPHYS_PAGES:
            *ret = -1;
            break;
        // Option queries. Answer honestly: -1 means "not supported", and a positive value
        // means supported. Getting these wrong makes callers take unsupported code paths.
        case _SC_VERSION:
        case _SC_2_VERSION:
        case _SC_XOPEN_VERSION:
            *ret = 200809L;
            break;
        case _SC_THREADS:
        case _SC_THREAD_SAFE_FUNCTIONS:
        case _SC_MONOTONIC_CLOCK:
        case _SC_BARRIERS:
        case _SC_SPIN_LOCKS:
        case _SC_READER_WRITER_LOCKS:
        case _SC_SEMAPHORES:
        case _SC_FSYNC:
        case _SC_MAPPED_FILES:
        case _SC_MEMORY_PROTECTION:
        case _SC_SPAWN:
        case _SC_REGEXP:
        case _SC_SHELL:
        case _SC_ADVISORY_INFO:
            *ret = 200809L;
            break;
        case _SC_JOB_CONTROL:
        case _SC_SAVED_IDS:
        case _SC_REALTIME_SIGNALS:
        case _SC_TIMERS:
        case _SC_ASYNCHRONOUS_IO:
        case _SC_PRIORITIZED_IO:
        case _SC_PRIORITY_SCHEDULING:
        case _SC_THREAD_PRIORITY_SCHEDULING:
        case _SC_MEMLOCK:
        case _SC_MEMLOCK_RANGE:
        case _SC_MESSAGE_PASSING:
        case _SC_SHARED_MEMORY_OBJECTS:
        case _SC_CPUTIME:
        case _SC_THREAD_CPUTIME:
        case _SC_TYPED_MEMORY_OBJECTS:
        case _SC_STREAMS:
            *ret = -1;
            break;
		default: {
            SYSTRACE("sys_sysconf: unhandled name %d", num);
			return EINVAL;
		}
	}
	SYSTRACE("sys_sysconf returning 0 (%ld)", *ret);
	return 0;
}
//
pid_t sys_getpid() {
    SYSTRACE("sys_getpid()");
	pid_t result = 1;
	SYSTRACE("sys_getpid returning %d", result);
	return result;
}

pid_t sys_gettid() {
    //SYSTRACE("sys_gettid()");
    struct thread_info info = twz_rt_get_thread_info(TWZ_RT_THREAD_ID_SELF);
	pid_t result = info.id;
	//SYSTRACE("sys_gettid returning %d", result);
	return result;
}

int sys_sigaltstack(const stack_t *ss, stack_t *oss) {
    SYSTRACE("sys_sigaltstack(ss=%p, oss=%p)", ss, oss);
    // Handlers run on the interrupted thread's own stack -- SA_ONSTACK is not honored -- so this
    // only records what was set and reports it back. Both arguments are independently optional:
    // sigaltstack(NULL, &old) queries and sigaltstack(&new, NULL) sets.
    // Not per-thread, unlike POSIX.
    static stack_t current = { .ss_sp = nullptr, .ss_flags = SS_DISABLE, .ss_size = 0 };

    if (ss && (ss->ss_flags & ~(SS_DISABLE | SS_ONSTACK))) {
        SYSTRACE("sys_sigaltstack returning EINVAL (bad ss_flags %d)", ss->ss_flags);
        return EINVAL;
    }
    if (ss && !(ss->ss_flags & SS_DISABLE) && ss->ss_size < MINSIGSTKSZ) {
        SYSTRACE("sys_sigaltstack returning ENOMEM (ss_size %ld)", ss->ss_size);
        return ENOMEM;
    }
    if (oss) {
        *oss = current;
    }
    if (ss) {
        current = *ss;
    }
	return 0;
}

int sys_getrlimit(int resource, struct rlimit *limit) {
    if(!limit)
        return EFAULT;
    switch (resource) {
        case RLIMIT_NOFILE:
            limit->rlim_cur = 1024; // Arbitrary limit for max file descriptors
            limit->rlim_max = 1024;
            return 0;
            case RLIMIT_STACK:
            limit->rlim_cur = 0x200000; // Default 2MB stack
            limit->rlim_max = 0x200000;
            return 0;
            case RLIMIT_CORE:
            limit->rlim_cur = 0; // No core dumps
            limit->rlim_max = 0;
            return 0;
        default:          
            limit->rlim_cur = (size_t)-1; // No limit on data segment
            limit->rlim_max = (size_t)-1;
            return 0;
    }
}

uid_t sys_getuid() {
    SYSTRACE("sys_getuid()");
	uid_t result = 0;
	SYSTRACE("sys_getuid returning %d", result);
	return result;
}

uid_t sys_geteuid() {
    SYSTRACE("sys_geteuid()");
	uid_t result = 0;
	SYSTRACE("sys_geteuid returning %d", result);
	return result;
}

gid_t sys_getgid() {
    SYSTRACE("sys_getgid()");
	gid_t result = 0;
	SYSTRACE("sys_getgid returning %d", result);
	return result;
}

gid_t sys_getegid() {
    SYSTRACE("sys_getegid()");
	gid_t result = 0;
	SYSTRACE("sys_getegid returning %d", result);
	return result;
}

int sys_kill(int pid, int sig) {
    SYSTRACE("sys_kill(pid=%d, sig=%d)", pid, sig);
	if(sig != 0 && !sigValid(sig))
		return EINVAL;
	// getpid() is always 1 and sys_spawn hands the child's descriptor back as its pid, so any
	// other value names a child compartment. Self-directed signals have to be dispatched here
	// rather than posted: raise()/abort() must take effect before returning.
	if(pid == sys_getpid()) {
		if(sig != 0)
			sigRaiseSelf(sig);
		SYSTRACE("sys_kill returning 0 (self)");
		return 0;
	}
	if(sig == 0) {
		// Existence probe; nothing to post.
		return 0;
	}
	uint64_t raw = (uint64_t)sig;
	int result = twz_error_errno(twz_rt_fd_set_config(pid, IO_REGISTER_SIGNAL, &raw, sizeof(raw)));
	SYSTRACE("sys_kill returning %d", result);
	return result;
}

void sys_thread_exit() {
    SYSTRACE("sys_thread_exit()");
    twz_rt_exit(0);
}

void sys_exit(int status) {
    SYSTRACE("sys_exit(status=%d)", status);
    twz_rt_exit(status);
}

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

int sys_futex_tid() {
	int result = sys_gettid();
	return result;
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
	struct option_duration timeout = NO_DURATION;
	if (time) {
		timeout.dur.seconds = (uint64_t)time->tv_sec;
		timeout.dur.nanos = (uint32_t)time->tv_nsec;
		timeout.is_some = 1;
	}
	twz_error err = twz_rt_futex_wait((_Atomic futex_word *)pointer, (futex_word)expected, timeout);
    return twz_error_errno(err);
}

int sys_futex_wake(int *pointer) {
	// twz_error is 64-bit with the category at bits 32-47 (ERROR_CATEGORY_SHIFT), so it must not
	// be narrowed to int before twz_error_errno() reads the category out of it.
	twz_error e = twz_rt_futex_wake((_Atomic futex_word *)pointer, FUTEX_WAKE_ALL);
	return twz_error_errno(e);
}

int sys_mkdir(const char *path, mode_t mode) {
    SYSTRACE("sys_mkdir(path=%s, mode=%o)", path, mode);
    int result = sys_mkdirat(AT_FDCWD, path, mode);
    SYSTRACE("sys_mkdir returning %d", result);
    return result;
}

int sys_mkdirat(int dirfd, const char *path, mode_t mode) {
    SYSTRACE("sys_mkdirat(dirfd=%d, path=%s, mode=%o)", dirfd, path, mode);
    if (dirfd != AT_FDCWD) {
        mlibc::sys_libc_log("sys_mkdirat: relative dirfd not supported");
        return ENOSYS;
    }
    twz_error err = twz_rt_fd_mkns(path, strlen(path));
    int result = twz_error_errno(err);
    SYSTRACE("sys_mkdirat returning %d", result);
    return result;
}

int sys_mknodat(int dirfd, const char *path, int mode, int dev) {
    SYSTRACE("sys_mknodat(dirfd=%d, path=%s, mode=%d, dev=%d)", dirfd, path, mode, dev);
	int result = ENOSYS;
	SYSTRACE("sys_mknodat returning %d", result);
	return result;
}

int sys_mkfifoat(int dirfd, const char *path, mode_t mode) {
    SYSTRACE("sys_mkfifoat(dirfd=%d, path=%s, mode=%o)", dirfd, path, mode);
	int result = ENOSYS;
	SYSTRACE("sys_mkfifoat returning %d", result);
	return result;
}

// sys_symlink and sys_symlinkat are implemented further down, with the other namespace ops.

// Stored but unused: nothing here creates files with a mode, so the mask gates nothing. Callers
// (mktemp, install) set it and restore it, and need the previous value back to do so.
static mode_t twz_umask = 022;

int sys_umask(mode_t mode, mode_t *old) {
    SYSTRACE("sys_umask(mode=%o, old=%p)", mode, old);
	if (old)
		*old = twz_umask;
	twz_umask = mode & 0777;
	return 0;
}

int sys_chdir(const char *path) {
    SYSTRACE("sys_chdir(path=%s)", path);
	int result = ENOSYS;
	SYSTRACE("sys_chdir returning %d", result);
	return result;
}

int sys_fchdir(int fd) {
    SYSTRACE("sys_fchdir(fd=%d)", fd);
	int result = ENOSYS;
	SYSTRACE("sys_fchdir returning %d", result);
	return result;
}

int sys_rename(const char *old_path, const char *new_path) {
    SYSTRACE("sys_rename(old_path=%s, new_path=%s)", old_path, new_path);
	twz_error err = twz_rt_fd_rename(old_path, strlen(old_path), new_path, strlen(new_path));
	int result = twz_error_errno(err);
	SYSTRACE("sys_rename returning %d", result);
	return result;
}

// sys_renameat is implemented further down, with the other namespace ops.

int sys_rmdir(const char *path) {
    SYSTRACE("sys_rmdir(path=%s)", path);
    twz_error err = twz_rt_fd_remove(path, strlen(path));
    int result = twz_error_errno(err);
    SYSTRACE("sys_rmdir returning %d", result);
    return result;
}

int sys_ftruncate(int fd, size_t size) {
    SYSTRACE("sys_ftruncate(fd=%d, size=%ld)", fd, size);
	uint64_t truncate_size = (uint64_t)size;
	twz_error err = twz_rt_fd_cmd(fd, FD_CMD_TRUNCATE, &truncate_size, NULL);
	int result = twz_error_errno(err);
	SYSTRACE("sys_ftruncate returning %d", result);
	return result;
}

int sys_readlink(const char *path, void *buf, size_t bufsiz, ssize_t *len) {
    SYSTRACE("sys_readlink(path=%s, buf=%p, bufsiz=%ld, len=%p)", path, buf, bufsiz, len);
    if(len) *len = 0;
    uint64_t out_len;
    twz_error err = twz_rt_fd_readlink(path, strlen(path), (char *)buf, bufsiz, &out_len);
    int result;
    if (err != 0) {
        result = twz_error_errno(err);
    } else if (out_len > SSIZE_MAX) {
        result = EOVERFLOW;
    } else {
        if(len) *len = (ssize_t)out_len;
        result = 0;
    }
    SYSTRACE("sys_readlink returning %d", result);
    return result;
}

pid_t sys_getppid() {
    SYSTRACE("sys_getppid()");
	pid_t result = 1;
	SYSTRACE("sys_getppid returning %d", result);
	return result;
}

int sys_setpgid(pid_t pid, pid_t pgid) {
    SYSTRACE("sys_setpgid(pid=%d, pgid=%d)", pid, pgid);
	int result = 0;
	SYSTRACE("sys_setpgid returning %d", result);
	return result;
}

int sys_getsid(pid_t pid, pid_t *sid) {
    SYSTRACE("sys_getsid(pid=%d, sid=%p)", pid, sid);
    // There is a single session, matching the fixed getpid() of 1. Returning 1 here would
    // mean EPERM and leave *sid untouched.
    if (!sid)
        return EFAULT;
    *sid = 1;
	SYSTRACE("sys_getsid returning 0 (sid=1)");
	return 0;
}

int sys_setsid(pid_t *sid) {
    SYSTRACE("sys_setsid(sid=%p)", sid);
	int result = 0;
	SYSTRACE("sys_setsid returning %d", result);
	return result;
}

int sys_setuid(uid_t uid) {
    SYSTRACE("sys_setuid(uid=%d)", uid);
	int result = 0;
	SYSTRACE("sys_setuid returning %d", result);
	return result;
}

int sys_setgid(gid_t gid) {
    SYSTRACE("sys_setgid(gid=%d)", gid);
	int result = 0;
	SYSTRACE("sys_setgid returning %d", result);
	return result;
}

int sys_getpgid(pid_t pid, pid_t *out) {
    SYSTRACE("sys_getpgid(pid=%d, out=%p)", pid, out);
    // Single process group, as with sys_getsid.
    if (!out)
        return EFAULT;
    *out = 1;
	SYSTRACE("sys_getpgid returning 0 (pgid=1)");
	return 0;
}

// No group model: the supplementary group list is genuinely empty, which is an answer rather
// than a refusal.
int sys_getgroups(size_t size, gid_t *list, int *retval) {
    SYSTRACE("sys_getgroups(size=%ld, list=%p, retval=%p)", size, list, retval);
	(void)size;
	(void)list;
	if (retval)
		*retval = 0;
	return 0;
}

int sys_dup(int fd, int flags, int *newfd) {
    SYSTRACE("sys_dup(fd=%d, flags=%d, newfd=%p)", fd, flags, newfd);
	if (!newfd) {
		int result = EFAULT;
		SYSTRACE("sys_dup returning %d", result);
		return result;
	}
	descriptor dup_fd;
	twz_error err = twz_rt_fd_cmd(fd, FD_CMD_DUP, NULL, &dup_fd);
	int result = twz_error_errno(err);
	if (result == 0) {
		// The duplicate refers to the same open file, so it inherits the recorded socket type
		// and access mode. FD_CLOEXEC is not inherited by dup() unless dup3 asked for it.
		fd_state_clear(dup_fd);
		fd_openflags_set(dup_fd, fd_openflags_get(fd));
		socket_prot_set(dup_fd, socket_type_get(fd));
		fd_cloexec_set(dup_fd, (flags & O_CLOEXEC) != 0);
		*newfd = dup_fd;
	}
	SYSTRACE("sys_dup returning %d", result);
	return result;
}

void sys_sync() {
    SYSTRACE("sys_sync()");
	// TODO
}

int sys_fsync(int fd) {
    SYSTRACE("sys_fsync(fd=%d)", fd);
    twz_error err = twz_rt_fd_cmd(fd, FD_CMD_SYNC, NULL, NULL);
    int result = twz_error_errno(err);
    SYSTRACE("sys_fsync returning %d", result);
    return result;
}

int sys_fdatasync(int fd) {
    SYSTRACE("sys_fdatasync(fd=%d)", fd);
    twz_error err = twz_rt_fd_cmd(fd, FD_CMD_SYNC, NULL, NULL);
    int result = twz_error_errno(err);
    SYSTRACE("sys_fdatasync returning %d", result);
    return result;
}

int sys_getrandom(void *buffer, size_t length, int flags, ssize_t *bytes_written) {
    SYSTRACE("sys_getrandom(buffer=%p, length=%ld, flags=%d, bytes_written=%p)", buffer, length, flags, bytes_written);
    if (!buffer || !bytes_written) {
        int result = EFAULT;
        SYSTRACE("sys_getrandom returning %d", result);
        return result;
    }

    // Convert mlibc flags to Twizzler flags
    // GRND_NONBLOCK = 1 from Linux getrandom man page
    get_random_flags twz_flags = 0;
    if (flags & 1) { // GRND_NONBLOCK
        twz_flags |= GET_RANDOM_NON_BLOCKING;
    }

    size_t bytes_read = twz_rt_get_random((char *)buffer, length, twz_flags);
    *bytes_written = (ssize_t)bytes_read;

    SYSTRACE("sys_getrandom returning 0, read %ld bytes", bytes_read);
    return 0;
}

int sys_getentropy(void *buffer, size_t length) {
    SYSTRACE("sys_getentropy(buffer=%p, length=%ld)", buffer, length);
    if (!buffer) {
        int result = EFAULT;
        SYSTRACE("sys_getentropy returning %d", result);
        return result;
    }

    // getentropy must fill the buffer completely, up to 256 bytes
    if (length > 256) {
        int result = EIO;
        SYSTRACE("sys_getentropy returning %d", result);
        return result;
    }

    // Use blocking mode (flags = 0) to ensure we get all the random data
    size_t bytes_read = twz_rt_get_random((char *)buffer, length, 0);
    
    if (bytes_read != length) {
        int result = EIO;
        SYSTRACE("sys_getentropy returning %d (got %ld bytes, expected %ld)", result, bytes_read, length);
        return result;
    }

    SYSTRACE("sys_getentropy returning 0");
    return 0;
}

} // namespace mlibc

#include <sys/file.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <sys/times.h>

namespace mlibc {

// ---------------------------------------------------------------------------
// Namespace operations. The underlying twz_rt_fd_* calls resolve names from the
// root, so only the AT_FDCWD forms of the *at() variants can be supported.
// ---------------------------------------------------------------------------

int sys_symlink(const char *target_path, const char *link_path) {
    SYSTRACE("sys_symlink(target_path=%s, link_path=%s)", target_path, link_path);
    if (!target_path || !link_path)
        return EFAULT;
    twz_error err = twz_rt_fd_symlink(link_path, strlen(link_path),
        target_path, strlen(target_path));
    int result = twz_error_errno(err);
    SYSTRACE("sys_symlink returning %d", result);
    return result;
}

int sys_symlinkat(const char *target_path, int dirfd, const char *link_path) {
    SYSTRACE("sys_symlinkat(target_path=%s, dirfd=%d, link_path=%s)", target_path, dirfd, link_path);
    if (dirfd != AT_FDCWD) {
        SYSTRACE("sys_symlinkat: relative dirfd not supported");
        return ENOSYS;
    }
    return sys_symlink(target_path, link_path);
}

int sys_readlinkat(int dirfd, const char *path, void *buffer, size_t max_size, ssize_t *length) {
    SYSTRACE("sys_readlinkat(dirfd=%d, path=%s, buffer=%p, max_size=%ld, length=%p)",
        dirfd, path, buffer, max_size, length);
    if (dirfd != AT_FDCWD) {
        SYSTRACE("sys_readlinkat: relative dirfd not supported");
        return ENOSYS;
    }
    return sys_readlink(path, buffer, max_size, length);
}

int sys_renameat(int old_dirfd, const char *old_path, int new_dirfd, const char *new_path) {
    SYSTRACE("sys_renameat(old_dirfd=%d, old_path=%s, new_dirfd=%d, new_path=%s)",
        old_dirfd, old_path, new_dirfd, new_path);
    if (old_dirfd != AT_FDCWD || new_dirfd != AT_FDCWD) {
        SYSTRACE("sys_renameat: relative dirfd not supported");
        return ENOSYS;
    }
    return sys_rename(old_path, new_path);
}

// ---------------------------------------------------------------------------
// Memory advice and locking. Everything here is advisory, and objects are
// already fully resident once mapped, so accepting these is accurate rather
// than merely convenient. Reporting ENOSYS instead breaks callers (allocators,
// CPython) that treat failure as fatal.
// ---------------------------------------------------------------------------

int sys_madvise(void *addr, size_t length, int advice) {
    SYSTRACE("sys_madvise(addr=%p, length=%ld, advice=%d)", addr, length, advice);
    return 0;
}

int sys_posix_madvise(void *addr, size_t length, int advice) {
    SYSTRACE("sys_posix_madvise(addr=%p, length=%ld, advice=%d)", addr, length, advice);
    return 0;
}

int sys_msync(void *addr, size_t length, int flags) {
    SYSTRACE("sys_msync(addr=%p, length=%ld, flags=%d)", addr, length, flags);
    // sys_vm_map only produces anonymous memory today, so there is nothing to write back.
    return 0;
}

int sys_mlock(const void *addr, size_t length) {
    SYSTRACE("sys_mlock(addr=%p, length=%ld)", addr, length);
    return 0;
}

int sys_munlock(const void *addr, size_t length) {
    SYSTRACE("sys_munlock(addr=%p, length=%ld)", addr, length);
    return 0;
}

int sys_mlockall(int flags) {
    SYSTRACE("sys_mlockall(flags=%d)", flags);
    return 0;
}

int sys_munlockall(void) {
    SYSTRACE("sys_munlockall()");
    return 0;
}

// ---------------------------------------------------------------------------
// File locking and allocation.
// ---------------------------------------------------------------------------

int sys_flock(int fd, int options) {
    SYSTRACE("sys_flock(fd=%d, options=%d)", fd, options);
    // No cross-compartment advisory locking exists yet. Validate the request so callers with
    // bad arguments still see EINVAL, then accept it.
    int op = options & ~LOCK_NB;
    if (op != LOCK_SH && op != LOCK_EX && op != LOCK_UN) {
        return EINVAL;
    }
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        return EBADF;
    }
    return 0;
}

int sys_fallocate(int fd, off_t offset, size_t size) {
    SYSTRACE("sys_fallocate(fd=%d, offset=%ld, size=%ld)", fd, offset, size);
    if (offset < 0) {
        return EINVAL;
    }
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        return EBADF;
    }
    // Only growing the file is expressible via FD_CMD_TRUNCATE; allocation is implicit for
    // object-backed files, so a range already within the file needs no work.
    uint64_t end = (uint64_t)offset + size;
    if (end <= info.len) {
        return 0;
    }
    twz_error err = twz_rt_fd_cmd(fd, FD_CMD_TRUNCATE, &end, NULL);
    int result = twz_error_errno(err);
    SYSTRACE("sys_fallocate returning %d", result);
    return result;
}

// ---------------------------------------------------------------------------
// Filesystem statistics. There is no notion of a mounted filesystem with a
// block budget, so the geometry fields are reported as unknown (0) rather than
// invented.
// ---------------------------------------------------------------------------

static void fill_statvfs(struct statvfs *out) {
    struct system_info si = twz_rt_get_sysinfo();
    memset(out, 0, sizeof(*out));
    out->f_bsize = si.page_size;
    out->f_frsize = si.page_size;
    out->f_namemax = NAME_MAX;
}

int sys_statvfs(const char *path, struct statvfs *out) {
    SYSTRACE("sys_statvfs(path=%s, out=%p)", path, out);
    if (!path || !out)
        return EFAULT;
    // Report per-path errors (e.g. ENOENT) the way a real statvfs would.
    int fd = -1;
    int e = sys_open(path, O_RDONLY, 0, &fd);
    if (e != 0) {
        SYSTRACE("sys_statvfs returning %d (open failed)", e);
        return e;
    }
    twz_rt_fd_close(fd);
    fill_statvfs(out);
    SYSTRACE("sys_statvfs returning 0");
    return 0;
}

int sys_fstatvfs(int fd, struct statvfs *out) {
    SYSTRACE("sys_fstatvfs(fd=%d, out=%p)", fd, out);
    if (!out)
        return EFAULT;
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        return EBADF;
    }
    fill_statvfs(out);
    SYSTRACE("sys_fstatvfs returning 0");
    return 0;
}

// ---------------------------------------------------------------------------
// Resource usage. The runtime exposes no per-process or per-thread CPU
// accounting, so elapsed wall time stands in for user time and the remaining
// counters are zero. This is approximate but lets callers that require the call
// to succeed (shell `times`, CPython's resource module) run.
// ---------------------------------------------------------------------------

int sys_getrusage(int scope, struct rusage *usage) {
    SYSTRACE("sys_getrusage(scope=%d, usage=%p)", scope, usage);
    if (!usage)
        return EFAULT;
    if (scope != RUSAGE_SELF && scope != RUSAGE_CHILDREN) {
        return EINVAL;
    }
    memset(usage, 0, sizeof(*usage));
    if (scope == RUSAGE_SELF) {
        struct duration up = twz_rt_get_monotonic_time();
        usage->ru_utime.tv_sec = (time_t)up.seconds;
        usage->ru_utime.tv_usec = (suseconds_t)(up.nanos / 1000);
    }
    SYSTRACE("sys_getrusage returning 0");
    return 0;
}

int sys_times(struct tms *tms, clock_t *out) {
    SYSTRACE("sys_times(tms=%p, out=%p)", tms, out);
    if (!tms || !out)
        return EFAULT;
    struct duration up = twz_rt_get_monotonic_time();
    clock_t ticks = (clock_t)(up.seconds * TWZ_CLK_TCK
        + up.nanos / (1000000000ul / TWZ_CLK_TCK));
    memset(tms, 0, sizeof(*tms));
    tms->tms_utime = ticks;
    *out = ticks;
    SYSTRACE("sys_times returning 0 (ticks=%ld)", (long)ticks);
    return 0;
}

// ---------------------------------------------------------------------------
// Process attributes that have a single fixed value in this environment.
// ---------------------------------------------------------------------------

int sys_setrlimit(int resource, const struct rlimit *limit) {
    SYSTRACE("sys_setrlimit(resource=%d, limit=%p)", resource, limit);
    if (!limit)
        return EFAULT;
    // Accepted and ignored, to match the fixed values sys_getrlimit reports.
    return 0;
}

static char hostname_buf[HOST_NAME_MAX + 1] = "twizzler";

int sys_sethostname(const char *buffer, size_t bufsize) {
    SYSTRACE("sys_sethostname(buffer=%p, bufsize=%ld)", buffer, bufsize);
    if (!buffer)
        return EFAULT;
    if (bufsize > HOST_NAME_MAX)
        return EINVAL;
    memcpy(hostname_buf, buffer, bufsize);
    hostname_buf[bufsize] = '\0';
    SYSTRACE("sys_sethostname returning 0 (%s)", hostname_buf);
    return 0;
}

int sys_getloadavg(double *samples) {
    SYSTRACE("sys_getloadavg(samples=%p)", samples);
    if (!samples)
        return EFAULT;
    // No load accounting exists.
    samples[0] = 0.0;
    samples[1] = 0.0;
    samples[2] = 0.0;
    return 0;
}

int sys_nice(int nice, int *new_nice) {
    SYSTRACE("sys_nice(nice=%d, new_nice=%p)", nice, new_nice);
    // Priorities are not adjustable from userspace yet; report an unchanged niceness.
    if (new_nice)
        *new_nice = 0;
    return 0;
}

int sys_sockatmark(int sockfd, int *out) {
    SYSTRACE("sys_sockatmark(sockfd=%d, out=%p)", sockfd, out);
    if (!out)
        return EFAULT;
    struct fd_info info;
    if (!twz_rt_fd_get_info(sockfd, &info)) {
        return EBADF;
    }
    if (info.kind != FdKind_Socket) {
        return ENOTSOCK;
    }
    // Out-of-band data is not supported, so the mark is never reached.
    *out = 0;
    return 0;
}

int sys_clock_set(int clock, time_t secs, long nanos) {
    SYSTRACE("sys_clock_set(clock=%d, secs=%ld, nanos=%ld)", clock, secs, nanos);
    // The system clock is not settable from a compartment.
    return EPERM;
}

// ---------------------------------------------------------------------------
// Scheduling parameters. The scheduler is not controllable through the runtime
// ABI, so these describe a single fixed policy rather than failing: pthread and
// CPython both query them during startup.
// ---------------------------------------------------------------------------

int sys_get_min_priority(int policy, int *out) {
    SYSTRACE("sys_get_min_priority(policy=%d, out=%p)", policy, out);
    if (!out)
        return EFAULT;
    if (policy != SCHED_OTHER && policy != SCHED_FIFO && policy != SCHED_RR)
        return EINVAL;
    *out = 0;
    return 0;
}

int sys_get_max_priority(int policy, int *out) {
    SYSTRACE("sys_get_max_priority(policy=%d, out=%p)", policy, out);
    if (!out)
        return EFAULT;
    if (policy != SCHED_OTHER && policy != SCHED_FIFO && policy != SCHED_RR)
        return EINVAL;
    *out = 0;
    return 0;
}

int sys_getscheduler(pid_t pid, int *policy) {
    SYSTRACE("sys_getscheduler(pid=%d, policy=%p)", pid, policy);
    if (!policy)
        return EFAULT;
    *policy = SCHED_OTHER;
    return 0;
}

int sys_getparam(pid_t pid, struct sched_param *param) {
    SYSTRACE("sys_getparam(pid=%d, param=%p)", pid, param);
    if (!param)
        return EFAULT;
    param->sched_priority = 0;
    return 0;
}

int sys_setparam(pid_t pid, const struct sched_param *param) {
    SYSTRACE("sys_setparam(pid=%d, param=%p)", pid, param);
    if (!param)
        return EFAULT;
    if (param->sched_priority != 0)
        return EINVAL;
    return 0;
}

int sys_getschedparam(void *tcb, int *policy, struct sched_param *param) {
    SYSTRACE("sys_getschedparam(tcb=%p, policy=%p, param=%p)", tcb, policy, param);
    if (!policy || !param)
        return EFAULT;
    *policy = SCHED_OTHER;
    param->sched_priority = 0;
    return 0;
}

int sys_setschedparam(void *tcb, int policy, const struct sched_param *param) {
    SYSTRACE("sys_setschedparam(tcb=%p, policy=%d, param=%p)", tcb, policy, param);
    if (!param)
        return EFAULT;
    if (policy != SCHED_OTHER)
        return EINVAL;
    if (param->sched_priority != 0)
        return EINVAL;
    return 0;
}

// ---------------------------------------------------------------------------
// CPU affinity. Threads are not pinnable through the runtime ABI, so queries
// report every CPU as eligible and requests are accepted without effect.
// ---------------------------------------------------------------------------

static int fill_all_cpus(size_t cpusetsize, cpu_set_t *mask) {
    if (!mask)
        return EFAULT;
    if (cpusetsize < sizeof(__cpu_mask) || (cpusetsize % sizeof(__cpu_mask)) != 0)
        return EINVAL;

    struct system_info si = twz_rt_get_sysinfo();
    size_t ncpus = si.available_parallelism ? si.available_parallelism : 1;
    size_t setbits = cpusetsize * 8;
    if (ncpus > setbits)
        ncpus = setbits;

    memset(mask, 0, cpusetsize);
    __cpu_mask *bits = mask->__bits;
    for (size_t i = 0; i < ncpus; i++) {
        bits[i / __NCPUBITS] |= (__cpu_mask)1 << (i % __NCPUBITS);
    }
    return 0;
}

int sys_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask) {
    SYSTRACE("sys_getaffinity(pid=%d, cpusetsize=%ld, mask=%p)", pid, cpusetsize, mask);
    return fill_all_cpus(cpusetsize, mask);
}

int sys_getthreadaffinity(pid_t tid, size_t cpusetsize, cpu_set_t *mask) {
    SYSTRACE("sys_getthreadaffinity(tid=%d, cpusetsize=%ld, mask=%p)", tid, cpusetsize, mask);
    return fill_all_cpus(cpusetsize, mask);
}

int sys_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
    SYSTRACE("sys_setaffinity(pid=%d, cpusetsize=%ld, mask=%p)", pid, cpusetsize, mask);
    if (!mask)
        return EFAULT;
    return 0;
}

int sys_setthreadaffinity(pid_t tid, size_t cpusetsize, const cpu_set_t *mask) {
    SYSTRACE("sys_setthreadaffinity(tid=%d, cpusetsize=%ld, mask=%p)", tid, cpusetsize, mask);
    if (!mask)
        return EFAULT;
    return 0;
}

// ---------------------------------------------------------------------------
// Network interfaces and terminal names.
// ---------------------------------------------------------------------------

int sys_if_nametoindex(const char *name, unsigned int *ret) {
    SYSTRACE("sys_if_nametoindex(name=%s, ret=%p)", name, ret);
    if (!name || !ret)
        return EFAULT;
    // The stack exposes no enumerable interfaces; only loopback is nameable.
    if (!strcmp(name, "lo")) {
        *ret = 1;
        return 0;
    }
    return ENODEV;
}

int sys_if_indextoname(unsigned int index, char *name) {
    SYSTRACE("sys_if_indextoname(index=%u, name=%p)", index, name);
    if (!name)
        return EFAULT;
    if (index != 1)
        return ENXIO;
    strcpy(name, "lo");
    return 0;
}

int sys_ttyname(int fd, char *buf, size_t size) {
    SYSTRACE("sys_ttyname(fd=%d, buf=%p, size=%ld)", fd, buf, size);
    if (!buf)
        return EFAULT;
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        return EBADF;
    }
    if (!(info.flags & FD_IS_TERMINAL)) {
        return ENOTTY;
    }
    // Terminals are not reachable by name, so report the conventional alias.
    static const char name[] = "/dev/tty";
    if (size < sizeof(name)) {
        return ERANGE;
    }
    memcpy(buf, name, sizeof(name));
    SYSTRACE("sys_ttyname returning 0");
    return 0;
}

} // namespace mlibc
#pragma clang diagnostic pop
