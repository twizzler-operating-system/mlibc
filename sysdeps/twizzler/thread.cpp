#include <mlibc/all-sysdeps.hpp>
#include <mlibc/thread.hpp>
#include <mlibc/threads.hpp>
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
#include "sysdeps.h"
#include <twizzler/rt/thread.h>
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
	tcb_ptr->selfPointer = tcb_ptr;
}

extern "C" void __mlibc_init_tcb(void *pointer) {
	initBasicTcb(reinterpret_cast<Tcb *>(pointer));
}


extern "C" void __mlibc_enter_thread(void *user_arg) {
	// entry points to twz_thread_args structure passed from sys_clone
	auto args = reinterpret_cast<struct twz_thread_args *>(user_arg);
	
	// Get the TCB that was already allocated by the runtime
	auto tcb = mlibc::get_current_tcb();
	
	// Initialize the TCB for this thread
	initBasicTcb(tcb);
	
	// Set the thread attributes that were passed from thread_create
	tcb->returnValueType = args->returns_int ? TcbThreadReturnValue::Integer : TcbThreadReturnValue::Pointer;
	tcb->isJoinable = args->is_joinable;
	
	// Extract the actual entry function and user_arg from the args structure
	void *actual_entry = args->entry;
	void *actual_user_arg = args->user_arg;
	//mlibc::infoLogger() << "Thread " << tcb->tid << " with args" << (void*)user_arg << "started with entry " << actual_entry << " and user_arg " << actual_user_arg << frg::endlog;
	
	// Wake any threads waiting for this thread to be created
	// (they may be waiting on the tid field in the thread handle)
	mlibc::sys_futex_wake(&tcb->tid);
	
	// Free the args structure since we're done with it
	getAllocator().free(args);
	
	// Invoke the actual thread function using the runtime's TCB
	tcb->invokeThreadFunc(actual_entry, actual_user_arg);
	
	// Mark that we've exited
	__atomic_store_n(&tcb->didExit, 1, __ATOMIC_RELEASE);
	mlibc::sys_futex_wake(&tcb->didExit);
	
	// Exit the thread
	mlibc::sys_thread_exit();
}


namespace mlibc {

extern "C" void __mlibc_handle_thread_exit(void *pointer, int ret_val) {
	run_dtors_for_tcb(reinterpret_cast<Tcb *>(pointer), ret_val);
}

static constexpr size_t default_stacksize = 0x200000;

int sys_prepare_stack(void **stack, void *entry, void *user_arg, size_t *stack_size, size_t *guard_size, void **stack_base) {
	SYSTRACE("sys_prepare_stack(stack=%p, entry=%p, user_arg=%p, stack_size=%p, guard_size=%p, stack_base=%p)", stack, entry, user_arg, stack_size, guard_size, stack_base);
	
	// The runtime handles stack and TCB allocation
	// We just need to store entry and user_arg for sys_clone to use
	if (!*stack_size)
		*stack_size = default_stacksize;
	
	return 0;
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
