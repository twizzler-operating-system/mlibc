#include "include/twizzler/rt/info.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/mman.h>

#include <type_traits>

#include <mlibc-config.h>
#include <bits/ensure.h>
#include <abi-bits/fcntl.h>
#include <abi-bits/socklen_t.h>
#include <mlibc/allocator.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <limits.h>

#include <twizzler/rt/object.h>
#include <twizzler/rt/fd.h>
#include <twizzler/rt/io.h>
#include <twizzler/rt/alloc.h>
#include <twizzler/rt/core.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

int twz_error_errno(uint64_t err) {
	if (err == 0) {
		return 0;
	}
	// TODO
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
	// TODO
	return 0;
}

int sys_anon_allocate(size_t size, void **pointer) {
    *pointer = twz_rt_malloc(size, 128, ZERO_MEMORY);
    if (*pointer == NULL) {
        return -1;
    }
    return 0;
}

int sys_anon_free(void *pointer, size_t size) {
    twz_rt_dealloc(pointer, size, 128, 0);
	return 0;
}

int sys_fadvise(int fd, off_t offset, off_t length, int advice) {
    // TODO
	return 0;
}

int sys_open(const char *path, int flags, mode_t mode, int *fd) {
    sys_libc_log("call to open");
    sys_libc_log(path);
	return ENOSYS;
}

int sys_openat(int dirfd, const char *path, int flags, mode_t mode, int *fd) {
    sys_libc_log("call to openat");
    sys_libc_log(path);
	return ENOSYS;
}

int sys_close(int fd) {
	// TODO
	return 0;
}

int sys_dup2(int fd, int flags, int newfd) {
	return ENOSYS;
}

int sys_read(int fd, void *buffer, size_t size, ssize_t *bytes_read) {
	return ENOSYS;
}

int sys_readv(int fd, const struct iovec *iovs, int iovc, ssize_t *bytes_read) {
	return ENOSYS;
}

int sys_write(int fd, const void *buffer, size_t size, ssize_t *bytes_written) {
	struct io_ctx ctx = {
		.flags = 0,
		.offset = FD_POS,
		.timeout = NO_DURATION,
	};
	struct io_result res = twz_rt_fd_pwrite((descriptor)fd, buffer, size, &ctx);
	if (res.err == SUCCESS) {
		if (bytes_written != nullptr) {
			*bytes_written = (ssize_t)res.val;
		}
		return 0;
	}
	return twz_error_errno(res.err);
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
    sys_libc_log("call to seek");
	// TODO
	return 0;
}

int sys_chmod(const char *pathname, mode_t mode) {
	return ENOSYS;
}

int sys_fchmod(int fd, mode_t mode) {
	return ENOSYS;
}

int sys_fchmodat(int fd, const char *pathname, mode_t mode, int flags) {
	return ENOSYS;
}

int sys_fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
	return ENOSYS;
}

int sys_utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags) {
	return ENOSYS;
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
	/*
	auto ret = do_syscall(SYS_munmap, pointer, size);
	if(int e = sc_error(ret); e)
		return e;
	return 0;
	*/
	return 0;
}

int sys_vm_protect(void *pointer, size_t size, int prot) {
	return 0;
}

// All remaining functions are disabled in ldso.
#ifndef MLIBC_BUILDING_RTLD

int sys_clock_get(int clock, time_t *secs, long *nanos) {
    // TODO
    *secs = 0;
    *nanos = 0;
    return 0;
}

int sys_clock_getres(int clock, time_t *secs, long *nanos) {
	return ENOSYS;
}

int sys_stat(fsfd_target fsfdt, int fd, const char *path, int flags, struct stat *statbuf) {
    sys_libc_log("call to stat");
	return ENOSYS;
}

int sys_statfs(const char *path, struct statfs *buf) {
    sys_libc_log("call to statfs");
	return ENOSYS;
}

int sys_fstatfs(int fd, struct statfs *buf) {
    sys_libc_log("call to fstatfs");
	return ENOSYS;
}

extern "C" void __mlibc_signal_restore(void);
extern "C" void __mlibc_signal_restore_rt(void);

int sys_sigaction(int signum, const struct sigaction *act,
		struct sigaction *oldact) {
	return 0;
}

int sys_socket(int domain, int type, int protocol, int *fd) {
	return ENOSYS;
}

int sys_msg_send(int sockfd, const struct msghdr *msg, int flags, ssize_t *length) {
	return ENOSYS;
}

ssize_t sys_sendto(int fd, const void *buffer, size_t size, int flags, const struct sockaddr *sock_addr, socklen_t addr_length, ssize_t *length) {
	return ENOSYS;
}

ssize_t sys_recvfrom(int fd, void *buffer, size_t size, int flags, struct sockaddr *sock_addr, socklen_t *addr_length, ssize_t *length) {
	return ENOSYS;
}

int sys_msg_recv(int sockfd, struct msghdr *msg, int flags, ssize_t *length) {
	return ENOSYS;
}

int sys_fcntl(int fd, int cmd, va_list args, int *result) {
    sys_libc_log("call to fcntl");
	return ENOSYS;
}

int sys_getcwd(char *buf, size_t size) {
    *buf = '/';
    *(buf + 1) = 0;
    return 0;
    //sys_libc_log("call to getcwd");
	//return ENOSYS;
}

int sys_unlinkat(int dfd, const char *path, int flags) {
	return ENOSYS;
}

int sys_sleep(time_t *secs, long *nanos) {
    *secs = 0;
    *nanos = 0;
    // TODO
	return 0;
}

int sys_isatty(int fd) {
	return 0;
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
	return ENOSYS;
}

int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	return ENOSYS;
}

int sys_pselect(int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask, int *num_events) {
	return ENOSYS;
}

int sys_pipe(int *fds, int flags) {
	return ENOSYS;
}

int sys_fork(pid_t *child) {
	return ENOSYS;
}

int sys_waitpid(pid_t pid, int *status, int flags, struct rusage *ru, pid_t *ret_pid) {
	return ENOSYS;
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
	return ENOSYS;
}

int sys_sigprocmask(int how, const sigset_t *set, sigset_t *old) {
	return ENOSYS;
}

int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid) {
	return ENOSYS;
}

int sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
	return ENOSYS;
}

int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid) {
	return ENOSYS;
}

int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid) {
	return ENOSYS;
}

int sys_setreuid(uid_t ruid, uid_t euid) {
	return ENOSYS;
}

int sys_setregid(gid_t rgid, gid_t egid) {
	return ENOSYS;
}

int sys_sysinfo(struct sysinfo *info) {
	return ENOSYS;
}

void sys_yield() {
	// TODO
}

int sys_clone(void *tcb, pid_t *pid_out, void *stack) {
	return ENOSYS;
}

extern "C" const char __mlibc_syscall_begin[1];
extern "C" const char __mlibc_syscall_end[1];

int sys_tgkill(int tgid, int tid, int sig) {
	return ENOSYS;
}

int sys_tcgetattr(int fd, struct termios *attr) {
	return ENOSYS;
}

int sys_tcsetattr(int fd, int optional_action, const struct termios *attr) {
	return ENOSYS;
}

int sys_tcflush(int fd, int queue) {
	return ENOSYS;
}

int sys_tcdrain(int fd) {
	return ENOSYS;
}

int sys_tcflow(int fd, int action) {
	return ENOSYS;
}

int sys_access(const char *path, int mode) {
    sys_libc_log("call to access");
	return ENOSYS;
}

int sys_faccessat(int dirfd, const char *pathname, int mode, int flags) {
    sys_libc_log("call to faccessat");
	return ENOSYS;
}

int sys_accept(int fd, int *newfd, struct sockaddr *addr_ptr, socklen_t *addr_length, int flags) {
	return ENOSYS;
}

int sys_bind(int fd, const struct sockaddr *addr_ptr, socklen_t addr_length) {
	return ENOSYS;
}

int sys_setsockopt(int fd, int layer, int number, const void *buffer, socklen_t size) {
	return ENOSYS;
}

int sys_sockname(int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length,
		socklen_t *actual_length) {
	return ENOSYS;
}

int sys_peername(int fd, struct sockaddr *addr_ptr, socklen_t max_addr_length,
		socklen_t *actual_length) {
	return ENOSYS;
}

int sys_listen(int fd, int backlog) {
	return ENOSYS;
}

int sys_shutdown(int sockfd, int how) {
	return ENOSYS;
}

int sys_getpriority(int which, id_t who, int *value) {
	return ENOSYS;
}

int sys_setpriority(int which, id_t who, int prio) {
	return ENOSYS;
}

int sys_open_dir(const char *path, int *fd) {
    sys_libc_log("call to open dir");
	return ENOSYS;
}

int sys_read_entries(int handle, void *buffer, size_t max_size, size_t *bytes_read) {
    sys_libc_log("call to read entries");
	return ENOSYS;
}

int sys_uname(struct utsname *buf) {
	return ENOSYS;
}

int sys_gethostname(char *buf, size_t bufsize) {
	return ENOSYS;
}

int sys_pread(int fd, void *buf, size_t n, off_t off, ssize_t *bytes_read) {
	return ENOSYS;
}

int sys_pwrite(int fd, const void *buf, size_t n, off_t off, ssize_t *bytes_written) {
	return ENOSYS;
}

int sys_getsockopt(int fd, int layer, int number, void *__restrict buffer, socklen_t *__restrict size) {
	return ENOSYS;
}

int sys_sysconf(int num, long *ret) {
    struct system_info info = twz_rt_get_sysinfo();
	switch(num) {
    	case _SC_NPROCESSORS_CONF:
            return info.available_parallelism;
    	case _SC_NPROCESSORS_ONLN:
    	    return info.available_parallelism;
        case _SC_PAGESIZE:
            return info.page_size;
		default: {
			return EINVAL;
		}
	}
	return 0;
}
#endif // __MLIBC_POSIX_OPTION
//
pid_t sys_getpid() {
	return ENOSYS;
}

pid_t sys_gettid() {
	return ENOSYS;
}

uid_t sys_getuid() {
	return ENOSYS;
}

uid_t sys_geteuid() {
	return ENOSYS;
}

gid_t sys_getgid() {
	return ENOSYS;
}

gid_t sys_getegid() {
	return ENOSYS;
}

int sys_kill(int pid, int sig) {
	return ENOSYS;
}

void sys_thread_exit() {
    twz_rt_exit(0);
}

void sys_exit(int status) {
    twz_rt_exit(status);
}

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

int sys_futex_tid() {
	return 1;
	//return ENOSYS;
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
	return 0;
	//return ENOSYS;
}

int sys_futex_wake(int *pointer) {
	return 0;
	//return ENOSYS;
}

int sys_mkdir(const char *path, mode_t mode) {
    sys_libc_log("call to mkdir");
	return ENOSYS;
}


int sys_mkdirat(int dirfd, const char *path, mode_t mode) {
    sys_libc_log("call to mkdirat");
	return ENOSYS;
}

int sys_mknodat(int dirfd, const char *path, int mode, int dev) {
	return ENOSYS;
}

int sys_mkfifoat(int dirfd, const char *path, mode_t mode) {
	return ENOSYS;
}

int sys_symlink(const char *target_path, const char *link_path) {
	return ENOSYS;
}

int sys_symlinkat(const char *target_path, int dirfd, const char *link_path) {
	return ENOSYS;
}

int sys_umask(mode_t mode, mode_t *old) {
	return ENOSYS;
}

int sys_chdir(const char *path) {
    sys_libc_log("call to chdir");
	return ENOSYS;
}

int sys_fchdir(int fd) {
    sys_libc_log("call to fchdir");
	return ENOSYS;
}

int sys_rename(const char *old_path, const char *new_path) {
	return ENOSYS;
}

int sys_renameat(int old_dirfd, const char *old_path, int new_dirfd, const char *new_path) {
	return ENOSYS;
}

int sys_rmdir(const char *path) {
	return ENOSYS;
}

int sys_ftruncate(int fd, size_t size) {
	return ENOSYS;
}

int sys_readlink(const char *path, void *buf, size_t bufsiz, ssize_t *len) {
	return ENOSYS;
}

pid_t sys_getppid() {
	return 1;
}

int sys_setpgid(pid_t pid, pid_t pgid) {
	return ENOSYS;
}

int sys_getsid(pid_t pid, pid_t *sid) {
	return 1;
}

int sys_setsid(pid_t *sid) {
	return ENOSYS;
}

int sys_setuid(uid_t uid) {
	return ENOSYS;
}

int sys_setgid(gid_t gid) {
	return ENOSYS;
}

int sys_getpgid(pid_t pid, pid_t *out) {
	return 1;
}

int sys_getgroups(size_t size, gid_t *list, int *retval) {
	return ENOSYS;
}

int sys_dup(int fd, int flags, int *newfd) {
	return ENOSYS;
}

void sys_sync() {
	// TODO
}

int sys_fsync(int fd) {
	return ENOSYS;
}

int sys_fdatasync(int fd) {
	return ENOSYS;
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
