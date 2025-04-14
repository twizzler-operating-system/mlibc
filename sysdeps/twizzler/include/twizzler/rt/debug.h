#pragma once

#include <stdint.h>
#include <stddef.h>
#include "handle.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Information about a loaded dynamic object
struct dl_phdr_info {
	/// Load address
	uintptr_t addr;
	/// Pointer to name, as a C string
	const char *name;
	/// Pointer to program headers
	const void *phdr;
	/// Number of program headers
  uint32_t phnum;
	unsigned long long int adds;
	unsigned long long int subs;
	size_t tls_modid;
	void *tls_data;
};

/// An ID for a loaded program image (or library)
typedef uint32_t loaded_image_id;

/// Information about a loaded program image or library
struct loaded_image {
	/// Object handle
  struct object_handle image_handle;
  /// Start of full image
  const void *image_start;
  /// Length of full image
  size_t image_len;
  /// The dl_info for this loaded image
  struct dl_phdr_info dl_info;
  /// The ID for this loaded image
  loaded_image_id id;
};

/// Get a loaded image from its ID. All IDs for loaded image are sequential, starting from TWZ_RT_EXEID.
/// On success, fill out data pointed to by the li argument and return true.
extern bool twz_rt_get_loaded_image(loaded_image_id id, struct loaded_image *li);

// This is for compatibility with C, and not intended for public use
extern int twz_rt_iter_phdr(int (*cb)(const struct dl_phdr_info *, size_t size, void *data), void *data);

/// The loaded image ID for the root loaded image (usually the executable)
const loaded_image_id TWZ_RT_EXEID = 0;

#ifdef __cplusplus
}
#endif
