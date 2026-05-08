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
    uint32_t open_flags = 0;
    if (flags & O_WRONLY) {
        open_flags |= OPEN_FLAG_WRITE;
    }
    if (flags & O_RDONLY) {
        open_flags |= OPEN_FLAG_READ;
    }
    if (flags & O_RDWR) {
        open_flags |= OPEN_FLAG_READ | OPEN_FLAG_WRITE;
    }
    if (flags & O_TRUNC) {
        open_flags |= OPEN_FLAG_TRUNCATE;
    }
    if (flags & O_APPEND) {
        open_flags |= OPEN_FLAG_TAIL;
    }
    //if (flags & O_SYMLINK) {
    //    open_flags |= OPEN_FLAG_SYMLINK;
    //}
    if (flags & O_SEARCH) {
        open_flags |= OPEN_FLAG_READ;
    }
    struct open_info args = {
        .create = co,
        .flags = 0,
        .len = strlen(path),
        .name = {}
    };
    memcpy(&args.name, path, args.len + 1);
    struct open_result res = twz_rt_fd_open(OpenKind_Path, open_flags, &args, sizeof(args));
    if (res.err != SUCCESS) {
        return twz_error_errno(res.err);
    }
    if(fd) {
        *fd = res.fd;
    }
    return 0;
}

int sys_close(int fd) {
    SYSTRACE("sys_close(fd=%d)", fd);
    twz_rt_fd_close(fd);
    int result = 0;
    SYSTRACE("sys_close returning %d", result);
    return result;
}

int sys_fcntl(int fd, int cmd, va_list args, int *result) {
    SYSTRACE("sys_fcntl(fd=%d, cmd=%d, result=%p)", fd, cmd, result);
	*result = 0;
	switch(cmd) {
		case F_GETFL:
			*result = O_RDWR;
			break;
	}
	return 0;
}

int sys_dup2(int fd, int flags, int newfd) {
    SYSTRACE("sys_dup2(fd=%d, flags=%d, newfd=%d)", fd, flags, newfd);
    
    if (fd == newfd) {
        // Duplicating to the same descriptor is a no-op
        SYSTRACE("sys_dup2 returning 0 (fd == newfd)");
        return 0;
    }
    
    // First, close the target descriptor if it's open
    twz_rt_fd_close(newfd);
    
    // Now duplicate the source descriptor
    descriptor dup_fd;
    twz_error err = twz_rt_fd_cmd(fd, FD_CMD_DUP, NULL, &dup_fd);
    int result = twz_error_errno(err);
    
    if (result != 0) {
        SYSTRACE("sys_dup2 returning %d (dup failed)", result);
        return result;
    }
    
    // If the duplicated descriptor is not the target descriptor, we need to close the dup
    // and try a different approach. In a real implementation with full fd control, we might
    // use dup2-specific syscalls, but Twizzler's FD_CMD_DUP doesn't guarantee a specific fd.
    // For now, we assume it returns the requested fd or we close and handle appropriately.
    if (dup_fd != newfd) {
        // The duplicated fd is not what we wanted, this would require more complex logic
        // In practice, Twizzler's fd management should handle this, but as a fallback
        // we close both and report an error
        twz_rt_fd_close(dup_fd);
        SYSTRACE("sys_dup2 returning %d (dup returned wrong fd)", EBADF);
        return EBADF;
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

int sys_chmod(const char *pathname, mode_t mode) {
    SYSTRACE("sys_chmod(pathname=%s, mode=%o)", pathname, mode);
	int result = ENOSYS;
	SYSTRACE("sys_chmod returning %d", result);
	return result;
}

int sys_fchmod(int fd, mode_t mode) {
    SYSTRACE("sys_fchmod(fd=%d, mode=%o)", fd, mode);
	int result = ENOSYS;
	SYSTRACE("sys_fchmod returning %d", result);
	return result;
}

int sys_fchmodat(int fd, const char *pathname, mode_t mode, int flags) {
    SYSTRACE("sys_fchmodat(fd=%d, pathname=%s, mode=%o, flags=%d)", fd, pathname, mode, flags);
	int result = ENOSYS;
	SYSTRACE("sys_fchmodat returning %d", result);
	return result;
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

int sys_vm_map(void *hint, size_t size, int prot, int flags,
		int fd, off_t offset, void **window) {
		//sys_libc_log("call to vm_map");
		if(!(flags & MAP_ANON) || fd != -1) {
		    return ENOTSUP;
		}
  *window = twz_rt_malloc(size, 0x1000, ZERO_MEMORY);
    if (*window == NULL) {
        return -1;
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
	/*
	auto ret = do_syscall(SYS_munmap, pointer, size);
	if(int e = sc_error(ret); e)
		return e;
	return 0;
	*/
	int result = 0;
	SYSTRACE("sys_vm_unmap returning %d", result);
	return result;
}

int sys_vm_protect(void *pointer, size_t size, int prot) {
    SYSTRACE("sys_vm_protect(pointer=%p, size=%ld, prot=%d)", pointer, size, prot);
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
        //case CLOCK_UPTIME:
        //case CLOCK_UPTIME_RAW:
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

    if (fsfdt == mlibc::fsfd_target::fd_path) {
        int e = sys_openat(fd, path, oflags, 0, &fd);
        if (e != 0) {
            return e;
        }
    } else if (fsfdt == mlibc::fsfd_target::path) {
        int e = sys_openat(AT_FDCWD, path, oflags, 0, &fd);
        if (e != 0) {
            return e;
        }
    }
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        return EBADF;
    }
    SYSTRACE("sys_stat: got fd info: mode=%o, len=%ld", info.unix_mode, info.len);
    statbuf->st_dev = 0;
    statbuf->st_ino = objid_to_ino(info.id);
    statbuf->st_mode = info.unix_mode | 0o777;
    statbuf->st_nlink = 1;
    statbuf->st_uid = 0;
    statbuf->st_gid = 0;
    statbuf->st_rdev = 0;
    statbuf->st_atime = info.accessed.seconds;
    statbuf->st_mtime = info.modified.seconds;
    statbuf->st_ctime = info.created.seconds;
    statbuf->st_size = info.len;
    return 0;
}

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

int sys_sigaction(int signum, const struct sigaction *act,
		struct sigaction *oldact) {
        SYSTRACE("sys_sigaction(signum=%d, act=%p, oldact=%p)", signum, act, oldact);
    if(oldact) {
        oldact->sa_handler = SIG_DFL;
        oldact->sa_flags = 0;
        sigemptyset(&oldact->sa_mask);
    }
	return 0;
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
            if (total_sent > 0) {
                if (length) *length = total_sent;
                SYSTRACE("sys_msg_send returning %ld (partial)", total_sent);
                return total_sent;
            }
            int result = twz_error_errno(res.err);
            SYSTRACE("sys_msg_send returning %d", result);
            return result;
        }
        
        total_sent += res.val;
    }
    
    if (length) *length = total_sent;
    SYSTRACE("sys_msg_send returning %ld", total_sent);
    return (int)total_sent;
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
            if (total_read > 0) {
                if (length) *length = total_read;
                SYSTRACE("sys_msg_recv returning %ld (partial)", total_read);
                return (int)total_read;
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
    SYSTRACE("sys_msg_recv returning %ld", total_read);
    return (int)total_read;
}


int sys_getcwd(char *buf, size_t size) {
    SYSTRACE("sys_getcwd(buf=%p, size=%ld)", buf, size);
    *buf = '/';
    *(buf + 1) = 0;
    int result = 0;
    SYSTRACE("sys_getcwd returning %d", result);
    return result;
    //sys_libc_log("call to getcwd");
	//return ENOSYS;
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

    switch(request) {
        case TIOCGWINSZ:
            return twz_error_errno(twz_rt_fd_get_config(fd, IO_REGISTER_WINSIZE, arg, sizeof(struct winsize)));
        default: *result = 0;
    }
	return 0;
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
    
    // Use Stream as default protocol (POSIX connect doesn't provide protocol info)
    enum prot_kind prot = ProtKind_Stream;
    
    // Reconnect the socket to the new address
    struct socket_bind_info bind_info = {.addr = twz_addr, .prot = prot};
    twz_error result = twz_rt_fd_reopen(sockfd, OpenKind_SocketConnect, OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        &bind_info, sizeof(bind_info));
    
    int retval = twz_error_errno(result);
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
	int result = 0;
	SYSTRACE("sys_sigprocmask returning %d", result);
	return result;
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

int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
    SYSTRACE("sys_getresuid(ruid=%p, euid=%p, suid=%p)", ruid, euid, suid);
	int result = ENOSYS;
	SYSTRACE("sys_getresuid returning %d", result);
	return result;
}

int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid) {
    SYSTRACE("sys_getresgid(rgid=%p, egid=%p, sgid=%p)", rgid, egid, sgid);
	int result = ENOSYS;
	SYSTRACE("sys_getresgid returning %d", result);
	return result;
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
	int result = ENOSYS;
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

    int fd;
    int e = sys_openat(dirfd, pathname, O_RDONLY, 0, &fd);
    if (e < 0) {
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
    
    // Use Stream as default protocol (POSIX bind doesn't provide protocol info)
    enum prot_kind prot = ProtKind_Stream;
    
    // Reopen the socket to bind to the specified address
    struct socket_bind_info bind_info = {.addr = twz_addr, .prot = prot};
    twz_error result = twz_rt_fd_reopen(fd, OpenKind_SocketBind, OPEN_FLAG_READ | OPEN_FLAG_WRITE,
        &bind_info, sizeof(bind_info));
    
    int retval = twz_error_errno(result);
    SYSTRACE("sys_bind returning %d", retval);
    return retval;
}

int sys_setsockopt(int fd, int layer, int number, const void *buffer, socklen_t size) {
    SYSTRACE("sys_setsockopt(fd=%d, layer=%d, number=%d, buffer=%p, size=%d)", fd, layer, number, buffer, size);
    
    if (!buffer) return EFAULT;
    
    // Map POSIX socket options to Twizzler register values
    if (layer == SOL_SOCKET) {
        switch(number) {
            case SO_RCVTIMEO: {
                struct timeval *tv = (struct timeval *)buffer;
                if (size < sizeof(struct timeval)) return EINVAL;
                struct option_duration dur = {
                    .dur = {.seconds = (uint64_t)tv->tv_sec, .nanos = (uint32_t)(tv->tv_usec * 1000)},
                    .is_some = 1
                };
                twz_error err = twz_rt_fd_set_config(fd, IO_REGISTER_READTIMEOUT, &dur, sizeof(dur));
                int result = twz_error_errno(err);
                SYSTRACE("sys_setsockopt returning %d", result);
                return result;
            }
            case SO_SNDTIMEO: {
                struct timeval *tv = (struct timeval *)buffer;
                if (size < sizeof(struct timeval)) return EINVAL;
                struct option_duration dur = {
                    .dur = {.seconds = (uint64_t)tv->tv_sec, .nanos = (uint32_t)(tv->tv_usec * 1000)},
                    .is_some = 1
                };
                twz_error err = twz_rt_fd_set_config(fd, IO_REGISTER_WRITETIMEOUT, &dur, sizeof(dur));
                int result = twz_error_errno(err);
                SYSTRACE("sys_setsockopt returning %d", result);
                return result;
            }
            default:
                SYSTRACE("sys_setsockopt returning %d (unsupported option)", EINVAL);
                return EINVAL;
        }
    } else if (layer == IPPROTO_TCP) {
        switch(number) {
            case TCP_NODELAY: {
                int nodelay = *(int *)buffer;
                uint32_t flags = nodelay ? SOCKET_FLAGS_NODELAY : 0;
                twz_error err = twz_rt_fd_set_config(fd, IO_REGISTER_SOCKET_FLAGS, &flags, sizeof(flags));
                int result = twz_error_errno(err);
                SYSTRACE("sys_setsockopt returning %d", result);
                return result;
            }
            default:
                SYSTRACE("sys_setsockopt returning %d (unsupported TCP option)", EINVAL);
                return EINVAL;
        }
    }
    
    SYSTRACE("sys_setsockopt returning %d (unsupported level)", EINVAL);
    return EINVAL;
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
    
    // Use FD_CMD_SHUTDOWN to shutdown read/write ends
    uint32_t shutdown_flags = how;  // SHUT_RD=0, SHUT_WR=1, SHUT_RDWR=2
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
    size_t dirent_size = 32;
	size_t nr_twz_entries = max_size / dirent_size;
	if(nr_twz_entries <= 0)
		nr_twz_entries = 1;
	SYSTRACE("sys_read_entries(handle=%d, buffer=%p, max_size=%ld, bytes_read=%p), off=%ld, nr_twz_entries=%ld",
		handle, buffer, max_size, bytes_read, off, nr_twz_entries);

	struct name_entry twznames[nr_twz_entries];
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
	snprintf(buf->machine, sizeof(buf->machine), "x86_64");

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
    if(layer != SOL_SOCKET) {
        SYSTRACE("sys_getsockopt returning %d (unsupported level)", EINVAL);
        return EINVAL;
    }
    switch(number) {
        case SO_ERROR:
            memset(buffer, 0, sizeof(int));
            *size = sizeof(int);
            SYSTRACE("sys_getsockopt returning 0, SO_ERROR=0");
            return 0;
        default:
	        SYSTRACE("sys_getsockopt returning EINVAL");
            return EINVAL;
    }
    return 0;
}

int sys_sysconf(int num, long *ret) {
    SYSTRACE("sys_sysconf(num=%d, ret=%p)", num, ret);
    struct system_info info = twz_rt_get_sysinfo();
	switch(num) {
    	case _SC_NPROCESSORS_CONF:
            *ret = info.available_parallelism;
            break;
    	case _SC_NPROCESSORS_ONLN:
    	    *ret = info.available_parallelism;
            break;
        case _SC_PAGESIZE:
            *ret = info.page_size;
            break;
		default: {
			return EINVAL;
		}
	}
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
    *oss = *ss;
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
	int result = ENOSYS;
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
	int e = twz_rt_futex_wake((_Atomic futex_word *)pointer, INT_MAX);
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

int sys_symlink(const char *target_path, const char *link_path) {
    SYSTRACE("sys_symlink(target_path=%s, link_path=%s)", target_path, link_path);
	int result = ENOSYS;
	SYSTRACE("sys_symlink returning %d", result);
	return result;
}

int sys_symlinkat(const char *target_path, int dirfd, const char *link_path) {
    SYSTRACE("sys_symlinkat(target_path=%s, dirfd=%d, link_path=%s)", target_path, dirfd, link_path);
	int result = ENOSYS;
	SYSTRACE("sys_symlinkat returning %d", result);
	return result;
}

int sys_umask(mode_t mode, mode_t *old) {
    SYSTRACE("sys_umask(mode=%o, old=%p)", mode, old);
	int result = ENOSYS;
	SYSTRACE("sys_umask returning %d", result);
	return result;
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
	int result = twz_rt_fd_rename(old_path, strlen(old_path), new_path, strlen(new_path));
	SYSTRACE("sys_rename returning %d", result);
	return result;
}

int sys_renameat(int old_dirfd, const char *old_path, int new_dirfd, const char *new_path) {
    SYSTRACE("sys_renameat(old_dirfd=%d, old_path=%s, new_dirfd=%d, new_path=%s)", old_dirfd, old_path, new_dirfd, new_path);
	int result = ENOSYS;
	SYSTRACE("sys_renameat returning %d", result);
	return result;
}

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
	int result = 1;
	SYSTRACE("sys_getsid returning %d", result);
	return result;
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
	int result = 1;
	SYSTRACE("sys_getpgid returning %d", result);
	return result;
}

int sys_getgroups(size_t size, gid_t *list, int *retval) {
    SYSTRACE("sys_getgroups(size=%ld, list=%p, retval=%p)", size, list, retval);
	int result = ENOSYS;
	SYSTRACE("sys_getgroups returning %d", result);
	return result;
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
#pragma clang diagnostic pop
