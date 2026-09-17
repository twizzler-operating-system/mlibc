#ifndef MLIBC_FRIGG_ALLOC
#define MLIBC_FRIGG_ALLOC

#include <mlibc/lock.hpp>
#include <bits/ensure.h>
#include <frg/slab.hpp>
#include <internal-config.h>

#if MLIBC_RT_ALLOCATOR

// Route the libc heap straight at the Twizzler runtime allocator instead of frigg's slab_pool.
//
// Why: frigg's `_construct_slab` partitions an entire 256 KiB slab up front, writing a freelist
// link into every object slot, so first use of any bucket touches 64 pages whether or not the
// program uses them. A kernel fault census attributed 21.9% of all faults in a guest `cargo build`
// to that one loop. The runtime allocator extends incrementally (ferroc bounds a shard extend to
// 4096 bytes), so it touches roughly the pages it actually hands out.
struct MemoryAllocator {
	void *allocate(size_t size);
	// Alignment-aware entry: frigg happened to return naturally-aligned blocks, which is what
	// posix_memalign relied on. Nothing here guarantees that, so the aligned path is explicit.
	void *allocate_aligned(size_t size, size_t align);
	void free(void *ptr);
	void deallocate(void *ptr, size_t size);
	void *reallocate(void *ptr, size_t size);
	size_t get_size(void *ptr);
};

MemoryAllocator &getAllocator();

#elif !MLIBC_DEBUG_ALLOCATOR

struct VirtualAllocator {
public:
	uintptr_t map(size_t length);

	void unmap(uintptr_t address, size_t length);
};

typedef frg::slab_pool<VirtualAllocator, FutexLock> MemoryPool;

typedef frg::slab_allocator<VirtualAllocator, FutexLock> MemoryAllocator;

MemoryAllocator &getAllocator();

#else

struct MemoryAllocator {
	void *allocate(size_t size);
	void free(void *ptr);
	void deallocate(void *ptr, size_t size);
	void *reallocate(void *ptr, size_t size);
	size_t get_size(void *ptr);
};

MemoryAllocator &getAllocator();

#endif // !MLIBC_DEBUG_ALLOCATOR

#endif // MLIBC_FRIGG_ALLOC
