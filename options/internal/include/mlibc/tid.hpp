#pragma once

#include <mlibc/thread.hpp>
#include <mlibc/internal-sysdeps.hpp>

namespace mlibc {
	inline unsigned int this_tid() {
		// During RTLD initialization, we don't have a TCB.
		if (mlibc::tcb_available_flag) {
			auto tcb = get_current_tcb();
			return tcb->tid;
		} else if (mlibc::sys_futex_tid) {
			unsigned int tid = mlibc::sys_futex_tid();
			return tid;
		} else {
			return 1;
		}
	}
}
