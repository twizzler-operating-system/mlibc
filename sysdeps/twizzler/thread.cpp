#include <mlibc/all-sysdeps.hpp>
#include <mlibc/thread.hpp>
#include <bits/ensure.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <frg/manual_box.hpp>
#include <frg/scope_exit.hpp>
#include <frg/small_vector.hpp>
#include <frg/unique.hpp>
#include <frg/hash_map.hpp>
#include <frg/optional.hpp>
#include <frg/string.hpp>
#include <frg/vector.hpp>
#include <frg/stack.hpp>
#include <frg/expected.hpp>
#include <mlibc/allocator.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-const-variable"


extern uintptr_t __stack_chk_guard;
static void initBasicTcb(Tcb *tcb_ptr) {
	tcb_ptr->stackCanary = __stack_chk_guard;
	tcb_ptr->cancelBits = tcbCancelEnableBit;
	tcb_ptr->didExit = 0;
	tcb_ptr->isJoinable = 1;
	memset(&tcb_ptr->returnValue, 0, sizeof(tcb_ptr->returnValue));
	tcb_ptr->localKeys = frg::construct<frg::array<Tcb::LocalKey, PTHREAD_KEYS_MAX>>(getAllocator());
}

extern "C" void __mlibc_init_tcb(Tcb *tcb_ptr) {
    initBasicTcb(tcb_ptr);
	// TODO
}

extern "C" void __mlibc_enter_thread(void *entry, void *user_arg) {
	// TODO
}

namespace mlibc {

static constexpr size_t default_stacksize = 0x200000;

int sys_prepare_stack(void **stack, void *entry, void *user_arg, void **tcb, size_t *stack_size, size_t *guard_size, void **stack_base) {
	return -ENOSYS;
}

// Declared in options/internal/mlibc/tcb.hpp.
#if !MLIBC_STATIC_BUILD && !MLIBC_BUILDING_RTLD
	// In non-static builds, libc.so always has a TCB available.
	//constexpr bool tcb_available_flag = true;
#else
	// Otherwise this will be set to true after RTLD has initialized the TCB.
	bool tcb_available_flag = true;
#endif
} // namespace mlibc
