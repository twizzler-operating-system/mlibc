#include <mlibc/all-sysdeps.hpp>
#include <mlibc/thread.hpp>
#include <bits/ensure.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-const-variable"

extern "C" void __mlibc_enter_thread(void *entry, void *user_arg) {
	// TODO
}

namespace mlibc {

static constexpr size_t default_stacksize = 0x200000;

int sys_prepare_stack(void **stack, void *entry, void *user_arg, void **tcb, size_t *stack_size, size_t *guard_size, void **stack_base) {
	return -ENOSYS;
}

// Declared in options/internal/mlibc/tcb.hpp.
bool tcb_available_flag = true;

} // namespace mlibc
