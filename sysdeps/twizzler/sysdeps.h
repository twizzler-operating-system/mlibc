
#include<stdio.h>
#include<twizzler/rt/io.h>
#include<twizzler/rt/fd.h>

extern "C" {
void __twz_enable_libc_trace(void);
extern bool _systrace;
}

#if 1
#include<stdio.h>
#define SYSTRACE(...) do { \
    if (!_systrace) { \
        _systrace = true; \
        char tbuf[258]; \
        snprintf(tbuf, 258, __VA_ARGS__); \
        strncat(tbuf, "\n", 258); \
        struct io_ctx ctx = {\
		.flags = 0,\
		.offset = FD_POS,\
		.timeout = NO_DURATION,\
	};\
        twz_rt_fd_pwrite(2, tbuf, strlen(tbuf), &ctx); \
        _systrace = false; \
    } \
    } while(0)
#else
#define SYSTRACE(...)
#endif


