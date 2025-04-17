sysdep_supported_options = {
	'posix': true,
	'linux': false,
	'glibc': true,
	'bsd': true,
}

rtld_dso_sources += files(
	'sysdeps.cpp',
)

libc_sources += files(
	'entry.cpp',
	'sysdeps.cpp',
)

if get_option('posix_option').allowed()
	libc_sources += files(
		'thread.cpp',
	)
endif

if not no_headers
	install_headers(
		'include/abi-bits/access.h',
		'include/abi-bits/auxv.h',
		'include/abi-bits/seek-whence.h',
		'include/abi-bits/vm-flags.h',
		'include/abi-bits/errno.h',
		'include/abi-bits/fcntl.h',
		'include/abi-bits/in.h',
		'include/abi-bits/stat.h',
		'include/abi-bits/statx.h',
		'include/abi-bits/signal.h',
		'include/abi-bits/reboot.h',
		'include/abi-bits/resource.h',
		'include/abi-bits/socket.h',
		'include/abi-bits/termios.h',
		'include/abi-bits/time.h',
		'include/abi-bits/blkcnt_t.h',
		'include/abi-bits/blksize_t.h',
		'include/abi-bits/dev_t.h',
		'include/abi-bits/gid_t.h',
		'include/abi-bits/ino_t.h',
		'include/abi-bits/mode_t.h',
		'include/abi-bits/nlink_t.h',
		'include/abi-bits/pid_t.h',
		'include/abi-bits/uid_t.h',
		'include/abi-bits/wait.h',
		'include/abi-bits/limits.h',
		'include/abi-bits/utsname.h',
		'include/abi-bits/ptrace.h',
		'include/abi-bits/poll.h',
		'include/abi-bits/epoll.h',
		'include/abi-bits/packet.h',
		'include/abi-bits/inotify.h',
		'include/abi-bits/clockid_t.h',
		'include/abi-bits/ipc.h',
		'include/abi-bits/shm.h',
		'include/abi-bits/mqueue.h',
		'include/abi-bits/suseconds_t.h',
		'include/abi-bits/fsfilcnt_t.h',
		'include/abi-bits/fsblkcnt_t.h',
		'include/abi-bits/socklen_t.h',
		'include/abi-bits/statfs.h',
		'include/abi-bits/statvfs.h',
		'include/abi-bits/ioctls.h',
		'include/abi-bits/xattr.h',
		'include/abi-bits/msg.h',
		'include/abi-bits/vt.h',
		'include/abi-bits/random.h',
		'include/abi-bits/rlim_t.h',
		'include/abi-bits/sigval.h',
		'include/abi-bits/sigevent.h',
		'include/twizzler/error.h',
		'include/twizzler/types.h',
		'include/twizzler/rt/alloc.h',
		'include/twizzler/rt/core.h',
		'include/twizzler/rt/debug.h',
		'include/twizzler/rt/fd.h',
		'include/twizzler/rt/handle.h',
		'include/twizzler/rt/info.h',
		'include/twizzler/rt/io.h',
		'include/twizzler/rt/object.h',
		'include/twizzler/rt/random.h',
		'include/twizzler/rt/thread.h',
		'include/twizzler/rt/time.h',
		'include/twizzler/rt/types.h',
		'include/twizzler/rt/version.h',
		subdir: 'abi-bits',
		follow_symlinks: true
	)

	install_headers(
		'include/twizzler/error.h',
		'include/twizzler/types.h',
		subdir: 'twizzler',
	)
	install_headers(
		'include/twizzler/rt/alloc.h',
		'include/twizzler/rt/core.h',
		'include/twizzler/rt/debug.h',
		'include/twizzler/rt/fd.h',
		'include/twizzler/rt/handle.h',
		'include/twizzler/rt/info.h',
		'include/twizzler/rt/io.h',
		'include/twizzler/rt/object.h',
		'include/twizzler/rt/random.h',
		'include/twizzler/rt/thread.h',
		'include/twizzler/rt/time.h',
		'include/twizzler/rt/types.h',
		'include/twizzler/rt/version.h',
		subdir: 'twizzler/rt',
	)
endif

if not headers_only
	wrapper_conf = configuration_data()
	wrapper_conf.set('LIBDIR', get_option('libdir'))
	wrapper_conf.set('PREFIX', get_option('prefix'))
	specs = configure_file(input: 'mlibc-gcc.specs.in',
			output: 'mlibc-gcc.specs',
			configuration: wrapper_conf)

	wrapper_script = configure_file(input: 'mlibc-gcc.in',
			output: 'mlibc-gcc',
			configuration: wrapper_conf)

	install_data(specs,
		install_dir: get_option('libdir')
	)

	install_data(wrapper_script,
		install_dir: get_option('bindir')
	)
endif
