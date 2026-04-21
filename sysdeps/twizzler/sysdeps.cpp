#include "include/twizzler/error.h"
#include "include/twizzler/rt/info.h"
#include "include/twizzler/rt/types.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <dirent.h>

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

#include <twizzler/rt/object.h>
#include <twizzler/rt/fd.h>
#include <twizzler/rt/io.h>
#include <twizzler/rt/alloc.h>
#include <twizzler/rt/core.h>
#include <twizzler/rt/thread.h>

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
        .flags = open_flags,
        .len = strlen(path),
        .name = {}
    };
    memcpy(&args.name, path, args.len + 1);
    struct open_result res = twz_rt_fd_open(OpenKind_Path, 0, &args, sizeof(args));
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

int sys_dup2(int fd, int flags, int newfd) {
    SYSTRACE("sys_dup2(fd=%d, flags=%d, newfd=%d)", fd, flags, newfd);
    int result = ENOSYS;
    SYSTRACE("sys_dup2 returning %d", result);
    return result;
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
    for(int i = 0; i < iovc; i++) {
        ssize_t thisread = 0;
        int e = sys_read(fd, iovs[i].iov_base, iovs[i].iov_len, &thisread);
        if (bytes_read != nullptr) {
			*bytes_read += thisread;
		}
        if (e != 0) {
            return e;
        }
    }
    return 0;
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
    // TODO
    *secs = 0;
    *nanos = 0;
    return 0;
}

int sys_thread_getname(void *tcb, char *name, size_t len) {
    SYSTRACE("sys_thread_getname(tcb=%p, name=%p, len=%ld)", tcb, name, len);
    twz_rt_get_name(tcb, name, &len);
    return 0;
}

int sys_clock_getres(int clock, time_t *secs, long *nanos) {
    SYSTRACE("sys_clock_getres(clock=%d, secs=%p, nanos=%p)", clock, secs, nanos);
	return ENOSYS;
}

int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
    SYSTRACE("sys_stat(fsfdt=%d, fd=%d, path=%s, flags=%d, statbuf=%p)", fsfdt, fd, path, flags, statbuf);
    if(flags & AT_SYMLINK_NOFOLLOW) {
        mlibc::sys_libc_log("symlink stat not supported in Twizzler");
        return ENOTSUP;
    }

    if (fsfdt == mlibc::fsfd_target::fd_path) {
        int e = sys_openat(fd, path, O_RDONLY, 0, &fd);
        if (e != 0) {
            return e;
        }
    } else if (fsfdt == mlibc::fsfd_target::path) {
        int e = sys_openat(AT_FDCWD, path, O_RDONLY, 0, &fd);
        if (e != 0) {
            return e;
        }
    }
    struct fd_info info;
    if (!twz_rt_fd_get_info(fd, &info)) {
        return EBADF;
    }
    statbuf->st_dev = 0;
    statbuf->st_ino = objid_to_ino(info.id);
    statbuf->st_mode = info.unix_mode;
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
	return 0;
}

int sys_socket(int domain, int type, int protocol, int *fd) {
    SYSTRACE("sys_socket(domain=%d, type=%d, protocol=%d, fd=%p)", domain, type, protocol, fd);
	int result = ENOSYS;
	SYSTRACE("sys_socket returning %d", result);
	return result;
}

int sys_msg_send(int sockfd, const struct msghdr *msg, int flags, ssize_t *length) {
    SYSTRACE("sys_msg_send(sockfd=%d, msg=%p, flags=%d, length=%p)", sockfd, msg, flags, length);
	int result = ENOSYS;
	SYSTRACE("sys_msg_send returning %d", result);
	return result;
}

ssize_t sys_sendto(int fd, const void *buffer, size_t size, int flags, const struct sockaddr *sock_addr, socklen_t addr_length, ssize_t *length) {
    SYSTRACE("sys_sendto(fd=%d, buffer=%p, size=%ld, flags=%d, sock_addr=%p, addr_length=%d, length=%p)", fd, buffer, size, flags, sock_addr, addr_length, length);
	return ENOSYS;
}

ssize_t sys_recvfrom(int fd, void *buffer, size_t size, int flags, struct sockaddr *sock_addr, socklen_t *addr_length, ssize_t *length) {
    SYSTRACE("sys_recvfrom(fd=%d, buffer=%p, size=%ld, flags=%d, sock_addr=%p, addr_length=%p, length=%p)", fd, buffer, size, flags, sock_addr, addr_length, length);
	return ENOSYS;
}

int sys_msg_recv(int sockfd, struct msghdr *msg, int flags, ssize_t *length) {
    SYSTRACE("sys_msg_recv(sockfd=%d, msg=%p, flags=%d, length=%p)", sockfd, msg, flags, length);
	int result = ENOSYS;
	SYSTRACE("sys_msg_recv returning %d", result);
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
	int result = ENOSYS;
	SYSTRACE("sys_unlinkat returning %d", result);
	return result;
}

int sys_sleep(time_t *secs, long *nanos) {
    SYSTRACE("sys_sleep(secs=%p, nanos=%p)", secs, nanos);
    *secs = 0;
    *nanos = 0;
    // TODO
	return 0;
}

int sys_isatty(int fd) {
    SYSTRACE("sys_isatty(fd=%d)", fd);
	int result = 0;
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
	int result = ENOSYS;
	SYSTRACE("sys_connect returning %d", result);
	return result;
}

int sys_pselect(int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask, int *num_events) {
        SYSTRACE("sys_pselect(nfds=%d, readfds=%p, writefds=%p, exceptfds=%p, timeout=%p, sigmask=%p, num_events=%p)",
            nfds, readfds, writefds, exceptfds, timeout, sigmask, num_events);
	return ENOSYS;
}

int sys_pipe(int *fds, int flags) {
    SYSTRACE("sys_pipe(fds=%p, flags=%d)", fds, flags);
	int result = ENOSYS;
	SYSTRACE("sys_pipe returning %d", result);
	return result;
}

int sys_fork(pid_t *child) {
    SYSTRACE("sys_fork(child=%p)", child);
	int result = ENOSYS;
	SYSTRACE("sys_fork returning %d", result);
	return result;
}

int sys_waitpid(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
    SYSTRACE("sys_waitpid(pid=%d, status=%p, flags=%d, ru=%p, ret_pid=%p)", pid, status, flags, ru, ret_pid);
	return ENOSYS;
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    SYSTRACE("sys_execve(path=%s, argv=%p, envp=%p)", path, argv, envp);
	int result = ENOSYS;
	SYSTRACE("sys_execve returning %d", result);
	return result;
}

int sys_sigprocmask(int how, const sigset_t *set, sigset_t *old) {
    SYSTRACE("sys_sigprocmask(how=%d, set=%p, old=%p)", how, set, old);
	int result = ENOSYS;
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
	int result = ENOSYS;
	SYSTRACE("sys_sysinfo returning %d", result);
	return result;
}

void sys_yield() {
    SYSTRACE("sys_yield()");
	// TODO
}

int sys_clone(void *tcb, pid_t *pid_out, void *stack) {
    SYSTRACE("sys_clone(tcb=%p, pid_out=%p, stack=%p)", tcb, pid_out, stack);
	int result = ENOSYS;
	SYSTRACE("sys_clone returning %d", result);
	return result;
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
	return 0;
}

int sys_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    SYSTRACE("sys_faccessat(dirfd=%d, pathname=%s, mode=%d, flags=%d)", dirfd, pathname, mode, flags);
	return 0;
}

int sys_accept(int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags) {
    SYSTRACE("sys_accept(fd=%d, newfd=%p, addr_ptr=%p, addr_length=%p, flags=%d)", fd, newfd, addr_ptr, addr_length, flags);
	int result = ENOSYS;
	SYSTRACE("sys_accept returning %d", result);
	return result;
}

int sys_bind(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
    SYSTRACE("sys_bind(fd=%d, addr_ptr=%p, addr_length=%d)", fd, addr_ptr, addr_length);
	int result = ENOSYS;
	SYSTRACE("sys_bind returning %d", result);
	return result;
}

int sys_setsockopt(int fd, int layer, int number, const void *buffer, socklen_t size) {
    SYSTRACE("sys_setsockopt(fd=%d, layer=%d, number=%d, buffer=%p, size=%d)", fd, layer, number, buffer, size);
	int result = ENOSYS;
	SYSTRACE("sys_setsockopt returning %d", result);
	return result;
}

int sys_sockname(int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length,
		socklen_t *actual_length) {
    SYSTRACE("sys_sockname(fd=%d, addr_ptr=%p, max_addr_length=%d, actual_length=%p)", fd, addr_ptr, max_addr_length, actual_length);
	int result = ENOSYS;
	SYSTRACE("sys_sockname returning %d", result);
	return result;
}

int sys_peername(int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length,
		socklen_t *actual_length) {
    SYSTRACE("sys_peername(fd=%d, addr_ptr=%p, max_addr_length=%d, actual_length=%p)", fd, addr_ptr, max_addr_length, actual_length);
	int result = ENOSYS;
	SYSTRACE("sys_peername returning %d", result);
	return result;
}

int sys_listen(int fd, int backlog) {
    SYSTRACE("sys_listen(fd=%d, backlog=%d)", fd, backlog);
	int result = ENOSYS;
	SYSTRACE("sys_listen returning %d", result);
	return result;
}

int sys_shutdown(int sockfd, int how) {
    SYSTRACE("sys_shutdown(sockfd=%d, how=%d)", sockfd, how);
	int result = ENOSYS;
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
		SYSTRACE("entry %ld: name=%.*s, ino=%ld, reclen=%hu, namelen=%d, direntsz = %ld, direntaln = %ld, thislen=%ld", i, entry->name_len, entry->name, target->d_ino, target->d_reclen, entry->name_len, dirent_size, thislen, 8);

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
	int result = ENOSYS;
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
	int result = ENOSYS;
	SYSTRACE("sys_getsockopt returning %d", result);
	return result;
}

int sys_sysconf(int num, long *ret) {
    SYSTRACE("sys_sysconf(num=%d, ret=%p)", num, ret);
    struct system_info info = twz_rt_get_sysinfo();
	switch(num) {
    	case _SC_NPROCESSORS_CONF:
            *ret = info.available_parallelism;
    	case _SC_NPROCESSORS_ONLN:
    	    *ret = info.available_parallelism;
        case _SC_PAGESIZE:
            *ret = info.page_size;
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
    SYSTRACE("sys_gettid()");
	pid_t result = 1;
	SYSTRACE("sys_gettid returning %d", result);
	return result;
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
	int result = 1;
	return result;
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
	int result = 0;
	return result;
	//return ENOSYS;
}

int sys_futex_wake(int *pointer) {
	int result = 0;
	return result;
	//return ENOSYS;
}

int sys_mkdir(const char *path, mode_t mode) {
    SYSTRACE("sys_mkdir(path=%s, mode=%o)", path, mode);
    sys_libc_log("call to mkdir");
	int result = ENOSYS;
	SYSTRACE("sys_mkdir returning %d", result);
	return result;
}


int sys_mkdirat(int dirfd, const char *path, mode_t mode) {
    SYSTRACE("sys_mkdirat(dirfd=%d, path=%s, mode=%o)", dirfd, path, mode);
    sys_libc_log("call to mkdirat");
	int result = ENOSYS;
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
	int result = ENOSYS;
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
	int result = ENOSYS;
	SYSTRACE("sys_rmdir returning %d", result);
	return result;
}

int sys_ftruncate(int fd, size_t size) {
    SYSTRACE("sys_ftruncate(fd=%d, size=%ld)", fd, size);
	int result = ENOSYS;
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
	int result = ENOSYS;
	SYSTRACE("sys_dup returning %d", result);
	return result;
}

void sys_sync() {
    SYSTRACE("sys_sync()");
	// TODO
}

int sys_fsync(int fd) {
    SYSTRACE("sys_fsync(fd=%d)", fd);
	int result = 0;
	SYSTRACE("sys_fsync returning %d", result);
	return result;
}

int sys_fdatasync(int fd) {
    SYSTRACE("sys_fdatasync(fd=%d)", fd);
	int result = 0;
	SYSTRACE("sys_fdatasync returning %d", result);
	return result;
}

int sys_getrandom(void *buffer, size_t length, int flags, ssize_t *bytes_written) {
    // TODO
    return 0;
}

int sys_getentropy(void *buffer, size_t length) {
    // TODO
    return 0;
}

} // namespace mlibc
#pragma clang diagnostic pop
