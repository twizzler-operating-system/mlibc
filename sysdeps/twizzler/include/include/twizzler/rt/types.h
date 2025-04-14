#pragma once
#include "../types.h"
#include "../error.h"
#include <stdint.h>
#include <stddef.h>

/// Error or value
struct u32_result {
  twz_error err;
  uint32_t val;
};

/// Error or value
struct u64_result {
  twz_error err;
  uint64_t val;
};

/// Error or value
struct objid_result {
  twz_error err;
  objid val;
};

/// Error or value
struct ptr_result {
  twz_error err;
  void *val;
};

/// Error or value
struct io_result {
  twz_error err;
  size_t val;
};


