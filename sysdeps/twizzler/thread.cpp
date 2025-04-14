#include <mlibc/all-sysdeps.hpp>
#include <mlibc/thread.hpp>
#include <bits/ensure.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

extern "C" void __mlibc_enter_thread(void *entry, void *user_arg) {
	// TODO
}

namespace mlibc {

static constexpr size_t default_stacksize = 0x200000;

int sys_prepare_stack(void **stack, void *entry, void *user_arg, void *tcb, size_t *stack_size, size_t *guard_size, void **stack_base) {
	return -ENOSYS;
}
} // namespace mlibc
