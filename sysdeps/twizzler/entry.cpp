#include <stdint.h>
#include <stdlib.h>
#include <bits/ensure.h>
#include <mlibc/elf/startup.h>
#include <sys/auxv.h>

extern char **environ;

size_t __hwcap;
uintptr_t *entryStack;

__attribute__((weak))
extern "C" void __mlibc_entry(uintptr_t *entry_stack, int (*main_fn)(int argc, char *argv[], char *env[])) {
	entryStack = entry_stack;
	__hwcap = getauxval(AT_HWCAP);
	if(main_fn != nullptr) {
    	auto result = main_fn(mlibc::entry_stack.argc, mlibc::entry_stack.argv, environ);
    	exit(result);
	}
}

extern "C" [[ gnu::visibility("default") ]] uintptr_t *__dlapi_entrystack() {
	return entryStack;
}

#pragma clang diagnostic pop
