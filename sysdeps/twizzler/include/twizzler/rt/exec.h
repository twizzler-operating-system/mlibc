#pragma once

#include "types.h"
#include "fd.h"

typedef uint32_t exec_flags;

struct exec_spawn_args {
    const char *prog;
    const char * const *args;
    const char * const *env;
    const struct binding_info *fd_binds;
    size_t fd_bind_count;
    exec_flags flags;
};

extern struct open_result twz_rt_exec_spawn(const struct exec_spawn_args *args);
