#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Exit code type
typedef int32_t exit_code;

/// Basic OS information provided to rust libstd
struct basic_aux {
  /// Number of arguments
  size_t argc;
  /// Pointer to args
  char **args;
  /// Environment pointer
  char **env;
};

/// Return info from rust libstd
struct basic_return {
  exit_code code;
};

struct ctor_set {
    /// Fn ptr to _init
    void (*legacy_init)();
    /// Pointer to the init array
    void (**init_array)();
    /// Length of init array
    size_t init_array_len;
};

/// Init information for compartments
struct comp_init_info {
  /// Pointer to ctor_set list
  struct ctor_set *ctor_set_array;
  /// Length of ctor_set array
  size_t ctor_set_len;
  /// Pointer to compartment config info
  void *comp_config_info;
};

/// Init information for minimal runtime
struct minimal_init_info {
  /// Pointer to program headers
  void *phdrs;
  /// Number of program headers
  size_t nr_phdrs;
};

/// Possible init info types
union init_info_ptrs {
    struct comp_init_info *comp;
    struct minimal_init_info *min;
    void *monitor;
};

/// Runtime initialization info.
struct runtime_info {
  int32_t flags;
  /// Discrim. for init_info.
  int32_t kind;
  /// Pointer to args
  char **args;
  /// Number of args
  size_t argc;
  /// Environment pointer
  char **envp;
  union init_info_ptrs init_info;
};

/// Minimal runtime info
const int32_t RUNTIME_INIT_MIN = 0;
/// Info for monitor
const int32_t RUNTIME_INIT_MONITOR = 1;
/// Info for compartments
const int32_t RUNTIME_INIT_COMP = 2;

/// Optional exit code
struct option_exit_code {
  int32_t is_some;
  exit_code value;
};

/// Exit with provided code
_Noreturn void twz_rt_exit(exit_code code);
/// Abort immediately
_Noreturn void twz_rt_abort(void);

/// Signal the runtime to prep for entry from another compartment
_Bool twz_rt_cross_compartment_entry(void);

/// Set the handler for an upcall from kernel
void twz_rt_set_upcall_handler(void (*handler)(void *frame, const void *data));

// The below functions are not meant for public use, they are for interacting with libstd.

struct option_exit_code twz_rt_pre_main_hook(void);
void twz_rt_post_main_hook(void);
_Noreturn void twz_rt_runtime_entry(const struct runtime_info *arg, struct basic_return (*std_entry)(struct basic_aux));

#ifdef __cplusplus
}
#endif
