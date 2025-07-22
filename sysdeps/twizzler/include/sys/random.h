
#ifndef _SYS_RANDOM_H
#define _SYS_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bits/ssize_t.h>
#include <bits/size_t.h>

#ifndef __MLIBC_ABI_ONLY

ssize_t getentropy(void *__buffer, size_t __max_size);

#endif /* !__MLIBC_ABI_ONLY */

#ifdef __cplusplus
}
#endif

#endif /*_SYS_RANDOM_H */
