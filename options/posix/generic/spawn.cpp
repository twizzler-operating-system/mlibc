
#include <spawn.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sched.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include <bits/ensure.h>
#include <mlibc/debug.hpp>

/*
 * Musl places this in a seperate header called fdop.h
 * This header isn't present in glibc, or on my host, so I
 * include it's contents here
 */

#define FDOP_CLOSE 1
#define FDOP_DUP2 2
#define FDOP_OPEN 3
#define FDOP_CHDIR 4
#define FDOP_FCHDIR 5

struct fdop {
	struct fdop *next, *prev;
	int cmd, fd, srcfd, oflag;
	mode_t mode;
	char path[];
};

/*
 * This posix_spawn implementation is taken from musl
 */

static unsigned long handler_set[NSIG / (8 * sizeof(long))];

static void __get_handler_set(sigset_t *set) {
	memcpy(set, handler_set, sizeof handler_set);
}

struct args {
	int p[2];
	sigset_t oldmask;
	const char *path;
	const posix_spawn_file_actions_t *fa;
	const posix_spawnattr_t *__restrict attr;
	char *const *argv, *const *envp;
};

static int child(void *args_vp) {
	int i, ret;
	struct sigaction sa = {};
	struct args *args = (struct args *)args_vp;
	int p = args->p[1];
	const posix_spawn_file_actions_t *fa = args->fa;
	const posix_spawnattr_t *__restrict attr = args->attr;
	sigset_t hset;
	bool use_execvpe = false;

	if(attr->__fn)
		use_execvpe = true;

	close(args->p[0]);

	/* All signal dispositions must be either SIG_DFL or SIG_IGN
	 * before signals are unblocked. Otherwise a signal handler
	 * from the parent might get run in the child while sharing
	 * memory, with unpredictable and dangerous results. To
	 * reduce overhead, sigaction has tracked for us which signals
	 * potentially have a signal handler. */
	__get_handler_set(&hset);
	for(i = 1; i < NSIG; i++) {
		if((attr->__flags & POSIX_SPAWN_SETSIGDEF) && sigismember(&attr->__def, i)) {
			sa.sa_handler = SIG_DFL;
		} else if(sigismember(&hset, i)) {
			if (i - 32 < 3) {
				sa.sa_handler = SIG_IGN;
			} else {;
				sigaction(i, nullptr, &sa);
				if(sa.sa_handler == SIG_IGN)
					continue;
				sa.sa_handler = SIG_DFL;
			}
		} else {
			continue;
		}
		sigaction(i, &sa, nullptr);
	}

	if(attr->__flags & POSIX_SPAWN_SETSID) {
		if((ret = setsid()) < 0)
			goto fail;
	}

	if(attr->__flags & POSIX_SPAWN_SETPGROUP) {
		mlibc::infoLogger() << "mlibc: posix_spawn: ignoring SETPGROUP" << frg::endlog;
		//if((ret = setpgid(0, attr->__pgrp)))
		//	goto fail;
	}

	if(attr->__flags & POSIX_SPAWN_RESETIDS) {
		if((ret = setgid(getgid())) || (ret = setuid(getuid())) )
			goto fail;
	}

	if(fa && fa->__actions) {
		struct fdop *op;
		int fd;
		for(op = (struct fdop *)fa->__actions; op->next; op = op->next);
		for(; op; op = op->prev) {
			/* It's possible that a file operation would clobber
			 * the pipe fd used for synchronizing with the
			 * parent. To avoid that, we dup the pipe onto
			 * an unoccupied fd. */
			if(op->fd == p) {
				ret = dup(p);
				if(ret < 0)
					goto fail;
				close(p);
				p = ret;
			}
			switch(op->cmd) {
			case FDOP_CLOSE:
				close(op->fd);
				break;
			case FDOP_DUP2:
				fd = op->srcfd;
				if(fd == p) {
					ret = -EBADF;
					goto fail;
				}
				if(fd != op->fd) {
					if((ret = dup2(fd, op->fd)) < 0)
						goto fail;
				} else {
					ret = fcntl(fd, F_GETFD);
					ret = fcntl(fd, F_SETFD, ret & ~FD_CLOEXEC);
					if(ret < 0)
						goto fail;
				}
				break;
			case FDOP_OPEN:
				fd = open(op->path, op->oflag, op->mode);
				if((ret = fd) < 0)
					goto fail;
				if(fd != op->fd) {
					if((ret = dup2(fd, op->fd)) < 0)
						goto fail;
					close(fd);
				}
				break;
			case FDOP_CHDIR:
				ret = chdir(op->path);
				if(ret < 0)
					goto fail;
				break;
			case FDOP_FCHDIR:
				ret = fchdir(op->fd);
				if(ret < 0)
					goto fail;
				break;
			}
		}
	}

	/* Close-on-exec flag may have been lost if we moved the pipe
	 * to a different fd. */
	fcntl(p, F_SETFD, FD_CLOEXEC);

	pthread_sigmask(SIG_SETMASK, (attr->__flags & POSIX_SPAWN_SETSIGMASK)
		? &attr->__mask : &args->oldmask, nullptr);

	if(use_execvpe)
		execvpe(args->path, args->argv, args->envp);
	else
		execve(args->path, args->argv, args->envp);
	ret = -errno;

fail:
	/* Since sizeof errno < PIPE_BUF, the write is atomic. */
	ret = -ret;
	if(ret)
		while(write(p, &ret, sizeof ret) < 0);
	_exit(127);
}

#if defined(__Twizzler__)
#include <string.h>

#include <twizzler/rt/exec.h>
#include <twizzler/rt/fd.h>

namespace mlibc {
int sys_spawn(int *, const char *, char *const*, char *const*);
}

// Defined in sysdeps/twizzler/sysdeps.cpp, which has no header of its own to declare it -- the
// twizzler include dir holds only verbatim copies of the ABI headers, so a declaration must not
// be added there. Same forward-declaration pattern as sys_spawn above.
int twz_error_errno(uint64_t err);

namespace {

// Twizzler has no fork, so there is no child context in which to replay file actions before the
// exec. They have to be resolved here, in the parent, into the fd_binds array twz_rt_exec_spawn
// takes -- the same array libstd's build_bindings constructs for Command's stdio redirects.
struct bind_set {
	binding_info *b = nullptr;
	// Parallel to b: entry was produced by a file action rather than inherited. Such an entry
	// names a descriptor that need not exist in this process, so its cloexec state cannot be
	// queried and must not be.
	char *made = nullptr;
	size_t n = 0, cap = 0;
};

static void bs_free(bind_set *bs) {
	free(bs->b);
	free(bs->made);
	bs->b = nullptr;
	bs->made = nullptr;
	bs->n = bs->cap = 0;
}

static int bs_grow(bind_set *bs, size_t want) {
	if(want <= bs->cap)
		return 0;
	size_t cap = bs->cap ? bs->cap : 8;
	while(cap < want)
		cap *= 2;
	auto nb = (binding_info *)realloc(bs->b, cap * sizeof(binding_info));
	if(!nb)
		return ENOMEM;
	bs->b = nb;
	auto nm = (char *)realloc(bs->made, cap);
	if(!nm)
		return ENOMEM;
	bs->made = nm;
	bs->cap = cap;
	return 0;
}

// Load every descriptor this process holds. Cloexec entries are deliberately included: in a
// fork+exec the child still has them while the file actions run, and only execve drops them, so
// a dup2 whose source is cloexec has to work. They are filtered at the end instead.
static int bs_load(bind_set *bs) {
	for(;;) {
		if(int e = bs_grow(bs, bs->cap ? bs->cap * 2 : 8))
			return e;
		size_t n = twz_rt_fd_read_binds(bs->b, bs->cap);
		if(n < bs->cap) {
			bs->n = n;
			memset(bs->made, 0, bs->cap);
			return 0;
		}
	}
}

static ssize_t bs_find(bind_set *bs, int fd) {
	for(size_t i = 0; i < bs->n; i++)
		if(bs->b[i].fd == fd)
			return (ssize_t)i;
	return -1;
}

static void bs_remove(bind_set *bs, int fd) {
	ssize_t i = bs_find(bs, fd);
	if(i < 0)
		return;
	bs->b[i] = bs->b[bs->n - 1];
	bs->made[i] = bs->made[bs->n - 1];
	bs->n--;
}

// Place a copy of `src` at descriptor `fd`, displacing whatever was there. Marks it as made, so
// the closing cloexec filter leaves it alone -- a redirect target is not an inherited descriptor.
static int bs_place(bind_set *bs, binding_info copy, int fd) {
	copy.fd = fd;
	ssize_t at = bs_find(bs, fd);
	if(at < 0) {
		if(int e = bs_grow(bs, bs->n + 1))
			return e;
		at = (ssize_t)bs->n++;
	}
	bs->b[at] = copy;
	bs->made[at] = 1;
	return 0;
}

// Resolve one posix_spawn file-action list and spawn. Actions are applied in the order they were
// added (the list is built in reverse, hence the walk to the tail and back), against a working
// copy of our binding set -- never against this process's real descriptor table, which a
// concurrent thread is entitled to be using.
static int twz_spawn_with_actions(pid_t *out_pid, const char *path,
		const posix_spawn_file_actions_t *fa, char *const argv[], char *const envp[]) {
	bind_set bs{};
	int *tmpfds = nullptr;
	size_t ntmp = 0;
	int ec = bs_load(&bs);

	if(!ec && fa && fa->__actions) {
		struct fdop *op;
		for(op = (struct fdop *)fa->__actions; op->next; op = op->next)
			;
		for(; op && !ec; op = op->prev) {
			switch(op->cmd) {
			case FDOP_CLOSE:
				bs_remove(&bs, op->fd);
				break;
			case FDOP_DUP2: {
				ssize_t i = bs_find(&bs, op->srcfd);
				if(i < 0) {
					ec = EBADF;
					break;
				}
				ec = bs_place(&bs, bs.b[i], op->fd);
				break;
			}
			case FDOP_OPEN: {
				// Opened here rather than in the child, because there is no child yet. The
				// descriptor is ours until the spawn completes, then closed.
				int fd = open(op->path, op->oflag, op->mode);
				if(fd < 0) {
					ec = errno;
					break;
				}
				auto nt = (int *)realloc(tmpfds, (ntmp + 1) * sizeof(int));
				if(!nt) {
					close(fd);
					ec = ENOMEM;
					break;
				}
				tmpfds = nt;
				tmpfds[ntmp++] = fd;

				bind_set fresh{};
				ec = bs_load(&fresh);
				if(!ec) {
					ssize_t i = bs_find(&fresh, fd);
					if(i < 0)
						ec = EBADF;
					else
						ec = bs_place(&bs, fresh.b[i], op->fd);
				}
				bs_free(&fresh);
				break;
			}
			case FDOP_CHDIR:
			case FDOP_FCHDIR:
				// Refused rather than ignored. There is no per-process cwd (sys_chdir is
				// ENOSYS), so accepting would hand the child a directory it is not in.
				ec = ENOSYS;
				break;
			default:
				ec = EINVAL;
				break;
			}
		}
	}

	// Now, and not before: a descriptor still marked close-on-exec does not cross. Entries a
	// file action produced are exempt -- those are redirect targets the caller asked for, and
	// dup2 clears the flag on its destination anyway. Same ordering as libstd's build_bindings,
	// and for the same reason: filtering earlier discards what was explicitly passed down.
	for(size_t i = 0; i < bs.n && !ec;) {
		uint32_t ce = 0;
		if(!bs.made[i] && twz_rt_fd_cmd(bs.b[i].fd, FD_CMD_GET_CLOEXEC, nullptr, &ce) == SUCCESS
				&& ce) {
			bs.b[i] = bs.b[bs.n - 1];
			bs.made[i] = bs.made[bs.n - 1];
			bs.n--;
		} else {
			i++;
		}
	}

	if(!ec) {
		struct exec_spawn_args sa = {
			.prog = path,
			.args = (const char *const *)argv,
			.env = (const char *const *)envp,
			.fd_binds = bs.b,
			.fd_bind_count = bs.n,
			.flags = 0,
		};
		struct open_result r = twz_rt_exec_spawn(&sa);
		if(r.err != SUCCESS)
			ec = twz_error_errno(r.err);
		else
			*out_pid = r.fd;
	}

	for(size_t i = 0; i < ntmp; i++)
		close(tmpfds[i]);
	free(tmpfds);
	bs_free(&bs);
	return ec;
}

} // namespace
#endif
int posix_spawn(pid_t *__restrict res, const char *__restrict path,
		const posix_spawn_file_actions_t *file_actions,
		const posix_spawnattr_t *__restrict attrs,
		char *const argv[], char *const envp[]) {
	pid_t pid;
	int ec = 0, cs;
	struct args args;
	const posix_spawnattr_t empty_attr = {};
	sigset_t full_sigset;
	sigfillset(&full_sigset);

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cs);

	args.path = path;
	args.fa = file_actions;
	args.attr = attrs ? attrs : &empty_attr;
	args.argv = argv;
	args.envp = envp;
	
#if defined(__Twizzler__)
	// On Twizzler, log unsupported flags but continue anyway
	if(attrs && (attrs->__flags & (POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK | 
	                              POSIX_SPAWN_SETSID | POSIX_SPAWN_RESETIDS | 
	                              POSIX_SPAWN_SETPGROUP))) {
		mlibc::infoLogger() << "mlibc: posix_spawn: ignoring unsupported flags on Twizzler" << frg::endlog;
	}
#else
	pthread_sigmask(SIG_BLOCK, &full_sigset, &args.oldmask);
#endif

#if defined(__Twizzler__)
	/* On Twizzler, use the Twizzler spawn API directly */
	// The fork path below captures the old mask as a side effect of blocking signals; this path
	// does not block (there is no shared-memory child to protect), so capture it explicitly.
	// Without this the restore at `fail:` installs whatever was on the stack.
	pthread_sigmask(SIG_SETMASK, nullptr, &args.oldmask);
	pid_t spawned_pid;
	int ret = twz_spawn_with_actions(&spawned_pid, path, file_actions, argv, envp);
	if(ret) {
		ec = ret;
		goto fail;
	}
	pid = spawned_pid;
#else
	/* The lock guards both against seeing a SIGABRT disposition change
	 * by abort and against leaking the pipe fd to fork-without-exec. */
	//LOCK(__abort_lock);

	if(pipe2(args.p, O_CLOEXEC)) {
		//UNLOCK(__abort_lock);
		ec = errno;
		goto fail;
	}

	/* Mlibc change: We use fork + execve, as clone is not implemented.
	 * This yields the same result in the end. */
	//pid = clone(child, stack + sizeof stack, CLONE_VM | CLONE_VFORK | SIGCHLD, &args);
	pid = fork();
	if(!pid) {
		child(&args);
	}
	close(args.p[1]);
	//UNLOCK(__abort_lock);

	if(pid > 0) {
		if(read(args.p[0], &ec, sizeof ec) != sizeof ec)
			ec = 0;
		else
			waitpid(pid, nullptr, 0);
	} else {
		ec = -pid;
	}

	close(args.p[0]);
#endif

	if(!ec && res)
		*res = pid;

fail:
	pthread_sigmask(SIG_SETMASK, &args.oldmask, nullptr);
	pthread_setcancelstate(cs, nullptr);

	return ec;
}

int posix_spawnattr_init(posix_spawnattr_t *attr) {
	*attr = (posix_spawnattr_t){};
	return 0;
}

int posix_spawnattr_destroy(posix_spawnattr_t *) {
	return 0;
}

int posix_spawnattr_setflags(posix_spawnattr_t *attr, short flags) {
	const unsigned all_flags = 
			POSIX_SPAWN_RESETIDS |
			POSIX_SPAWN_SETPGROUP |
			POSIX_SPAWN_SETSIGDEF |
			POSIX_SPAWN_SETSIGMASK |
			POSIX_SPAWN_SETSCHEDPARAM |
			POSIX_SPAWN_SETSCHEDULER |
			POSIX_SPAWN_USEVFORK |
			POSIX_SPAWN_SETSID;
	if(flags & ~all_flags)
		return EINVAL;
	attr->__flags = flags;
	return 0;
}

int posix_spawnattr_setsigdefault(posix_spawnattr_t *__restrict attr,
		const sigset_t *__restrict sigdefault) {
	attr->__def = *sigdefault;
	return 0;
}

int posix_spawnattr_setschedparam(posix_spawnattr_t *__restrict,
		const struct sched_param *__restrict) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setschedpolicy(posix_spawnattr_t *, int) {
	__ensure(!"Not implemented");
	__builtin_unreachable();
}

int posix_spawnattr_setsigmask(posix_spawnattr_t *__restrict attr,
		const sigset_t *__restrict sigmask) {
	attr->__mask = *sigmask;
	return 0;
}

int posix_spawnattr_setpgroup(posix_spawnattr_t *attr, pid_t pgroup) {
	attr->__pgrp = pgroup;
	return 0;
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *file_actions) {
	file_actions->__actions = nullptr;
	return 0;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *file_actions) {
	struct fdop *op = (struct fdop *)file_actions->__actions, *next;
	while(op) {
		next = op->next;
		free(op);
		op = next;
	}
	return 0;
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *file_actions,
		int fildes, int newfildes) {
	struct fdop *op = (struct fdop *)malloc(sizeof *op);
	if(!op)
		return ENOMEM;
	op->cmd = FDOP_DUP2;
	op->srcfd = fildes;
	op->fd = newfildes;
	if((op->next = (struct fdop *)file_actions->__actions))
		op->next->prev = op;
	op->prev = nullptr;
	file_actions->__actions = op;
	return 0;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *file_actions,
		int fildes) {
	struct fdop *op = (struct fdop *)malloc(sizeof *op);
	if(!op)
		return ENOMEM;
	op->cmd = FDOP_CLOSE;
	op->fd = fildes;
	if((op->next = (struct fdop *)file_actions->__actions))
		op->next->prev = op;
	op->prev = nullptr;
	file_actions->__actions = op;
	return 0;
}

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *__restrict file_actions,
		int fildes, const char *__restrict path, int oflag, mode_t mode) {
	struct fdop *op = (struct fdop *)malloc(sizeof *op + strlen(path) + 1);
	if(!op)
		return ENOMEM;
	op->cmd = FDOP_OPEN;
	op->fd = fildes;
	op->oflag = oflag;
	op->mode = mode;
	strcpy(op->path, path);
	if((op->next = (struct fdop *)file_actions->__actions))
		op->next->prev = op;
	op->prev = nullptr;
	file_actions->__actions = op;
	return 0;
}

int posix_spawnp(pid_t *__restrict pid, const char *__restrict file,
		const posix_spawn_file_actions_t *file_actions,
		const posix_spawnattr_t *__restrict attrp,
		char *const argv[], char *const envp[]) {
	posix_spawnattr_t spawnp_attr = {};
	if(attrp)
		spawnp_attr = *attrp;
	spawnp_attr.__fn = (void *)execvpe;	
	return posix_spawn(pid, file, file_actions, &spawnp_attr, argv, envp);
}

