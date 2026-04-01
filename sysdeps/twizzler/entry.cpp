#include <stdint.h>
#include <stdlib.h>
#include <bits/ensure.h>
#include <mlibc/elf/startup.h>
#include <sys/auxv.h>

extern char **environ;

size_t __hwcap;
uintptr_t *entryStack;

extern "C" void __rust_entry_from_c(uintptr_t arg, int (*main_fn)(int argc, char *argv[], char *env[]));

__attribute__((weak))
extern "C" void __mlibc_entry(uintptr_t arg, int (*main_fn)(int argc, char *argv[], char *env[])) {
    __rust_entry_from_c(arg, main_fn);
}

__attribute__((weak))
extern "C" void __mlibc_entry_from_rust(uintptr_t *entry_stack, int (*main_fn)(int argc, char *argv[], char *env[])) {
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
