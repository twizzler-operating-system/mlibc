#pragma once

#include <mlibc/thread.hpp>
#include <mlibc/internal-sysdeps.hpp>

namespace mlibc {
	inline unsigned int this_tid() {
		// During RTLD initialization, we don't have a TCB.
		if (mlibc::tcb_available_flag) {
			auto tcb = get_current_tcb();
			// tid 0 means this thread's tid was never populated: the runtime creates threads
			// without going through pthread_create, and (on Twizzler) initializes their TCB
			// from the *spawning* thread, which cannot know the child's tid. Running the mutex
			// protocol with tid 0 silently corrupts it -- a tid-0 lock CASes 0->0, leaving the
			// word looking unlocked while setting __mlibc_recursion. Populate on first use,
			// from the thread itself, where the answer is always the right one.
			if (tcb->tid == 0 && mlibc::sys_futex_tid) {
				tcb->tid = mlibc::sys_futex_tid();
			}
			return tcb->tid;
		} else if (mlibc::sys_futex_tid) {
			unsigned int tid = mlibc::sys_futex_tid();
			return tid;
		} else {
			return 1;
		}
	}
}
