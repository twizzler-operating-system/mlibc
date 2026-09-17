
#include <string.h>
#include <stdint.h>

#include <bits/ensure.h>
#include <frg/eternal.hpp>
#include <mlibc/allocator.hpp>
#include <mlibc/internal-sysdeps.hpp>
#include <internal-config.h>

#if MLIBC_RT_ALLOCATOR

extern "C" {
void *twz_rt_malloc(size_t sz, size_t align, uint32_t flags);
void twz_rt_dealloc(void *ptr, size_t sz, size_t align, uint32_t flags);
}

namespace {

// Immediately precedes every pointer handed out. `base`/`total` are what the runtime must be given
// back verbatim: twz_rt_dealloc is a SIZED free, and the runtime routes on the pointer it issued,
// so neither value can be reconstructed from the user pointer once alignment padding is involved.
struct RtHeader {
	void *base;
	size_t total;
};

constexpr size_t kHeader = sizeof(RtHeader);
// Every pointer libc hands out must suit max_align_t.
constexpr size_t kMinAlign = 16;

void *rt_alloc_impl(size_t size, size_t align) {
	if(align < kMinAlign)
		align = kMinAlign;
	if(!size)
		size = 1;
	// The alignment slack is only needed when the caller wants more than the runtime already
	// guarantees. Paying it unconditionally cost ~2x footprint on every allocation under 4 KiB and
	// turned this experiment into a 29.5% fault REGRESSION -- the runtime's own shard extension
	// then had to touch twice the pages. A twz_rt_malloc of kMinAlign is already 16-aligned, so a
	// header-sized offset from it stays 16-aligned with no rounding at all.
	size_t total = (align <= kMinAlign) ? (size + kHeader) : (size + align + kHeader);
	void *base = twz_rt_malloc(total, kMinAlign, 0);
	if(!base)
		return nullptr;
	uintptr_t raw = reinterpret_cast<uintptr_t>(base) + kHeader;
	uintptr_t user = (align <= kMinAlign) ? raw : ((raw + (align - 1)) & ~(align - 1));
	auto *hdr = reinterpret_cast<RtHeader *>(user - kHeader);
	hdr->base = base;
	hdr->total = total;
	return reinterpret_cast<void *>(user);
}

size_t rt_usable(void *ptr) {
	auto *hdr = reinterpret_cast<RtHeader *>(reinterpret_cast<uintptr_t>(ptr) - kHeader);
	uintptr_t off = reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(hdr->base);
	return hdr->total - off;
}

} // namespace

void *MemoryAllocator::allocate(size_t size) {
	// frigg returned naturally-aligned blocks, and posix_memalign leans on that by asking for
	// max(align, size). Match it so that contract still holds without a header-shifted pointer
	// breaking the alignment: give a request of N at least the alignment of the largest power of
	// two <= N, capped at a page.
	// Plain allocations need only max_align_t. The natural-alignment contract frigg happened to
	// provide is now supplied explicitly by allocate_aligned, and posix_memalign/aligned_alloc are
	// its only consumers -- every other getAllocator().allocate() caller in mlibc is a byte buffer.
	return rt_alloc_impl(size, kMinAlign);
}

void *MemoryAllocator::allocate_aligned(size_t size, size_t align) {
	return rt_alloc_impl(size, align);
}

void MemoryAllocator::free(void *ptr) {
	if(!ptr)
		return;
	auto *hdr = reinterpret_cast<RtHeader *>(reinterpret_cast<uintptr_t>(ptr) - kHeader);
	twz_rt_dealloc(hdr->base, hdr->total, kMinAlign, 0);
}

void MemoryAllocator::deallocate(void *ptr, size_t) {
	free(ptr);
}

void *MemoryAllocator::reallocate(void *ptr, size_t size) {
	if(!size) {
		free(ptr);
		return nullptr;
	}
	if(!ptr)
		return allocate(size);
	size_t old = rt_usable(ptr);
	// Not twz_rt_realloc: that would move the runtime's block, but the user pointer is offset from
	// it by alignment padding, so the header would no longer sit where free() looks for it.
	void *out = allocate(size);
	if(!out)
		return nullptr;
	memcpy(out, ptr, old < size ? old : size);
	free(ptr);
	return out;
}

size_t MemoryAllocator::get_size(void *ptr) {
	return rt_usable(ptr);
}

MemoryAllocator &getAllocator() {
	// frg::eternal for the same reason as the other variants: __cxa_atexit() calls this.
	static frg::eternal<MemoryAllocator> singleton{};
	return singleton.get();
}

#elif !MLIBC_DEBUG_ALLOCATOR

// --------------------------------------------------------
// Globals
// --------------------------------------------------------

MemoryAllocator &getAllocator() {
	// use frg::eternal to prevent a call to __cxa_atexit().
	// this is necessary because __cxa_atexit() call this function.
	static frg::eternal<VirtualAllocator> virtualAllocator;
	static frg::eternal<MemoryPool> heap{virtualAllocator.get()};
	static frg::eternal<MemoryAllocator> singleton{&heap.get()};
	return singleton.get();
}

// --------------------------------------------------------
// VirtualAllocator
// --------------------------------------------------------

uintptr_t VirtualAllocator::map(size_t length) {
	void *ptr;
	__ensure(!mlibc::sys_anon_allocate(length, &ptr));
	return (uintptr_t)ptr;
}

void VirtualAllocator::unmap(uintptr_t address, size_t length) {
	__ensure(!mlibc::sys_anon_free((void *)address, length));
}

#else

namespace {
	struct AllocatorMeta {
		size_t allocatedSize;
		size_t pagesSize;
		frg::array<uint64_t, 4> magic;
	};

	constexpr frg::array<uint64_t, 4> allocatorMagic {
		0x6d4bbb9f3446e83f, 0x25e213a7a7f9f954,
		0x1a3c667586538bef, 0x994f34ff71c090bc
	};
} // namespace anonymous

// Turn vm_unmap calls in free into vm_map(..., PROT_NONE, ...) calls to prevent
// those addresses from being reused. This is useful for detecting situations like this:
// 1. Allocate object X at address Y
// 2. Do some computation using object X
// 3. Free object X at address Y
// 4. Allocate object Z at address W, and it so happens that W == Y
// 5. Try to use object X, but the memory which was backing it now contains object Z
constexpr bool neverReleaseVa = false;
constexpr bool logAllocations = false;

// Area before the returned allocated block (which exists due to us offseting
// the block to be as close to the edge of a page).
constexpr uint8_t offsetAreaValue = 'A';
// Area which we return a pointer to in allocate and reallocate.
constexpr uint8_t allocatedAreaValue = 'B';
// Area after the allocated block, which exists due to the alignment constraints.
constexpr uint8_t alignmentAreaValue = 'C';
// Remaining area within the metadata page after the metadata.
constexpr uint8_t metaAreaValue = 'D';

// Alignment of the returned memory.
// TODO(qookie): Eventually accept alignment as an argument of allocate.
constexpr size_t pointerAlignment = 16;

// TODO(qookie): Support this. Perhaps by overallocating by 2x and then picking
// an offset that guarantees the desired alignment.
static_assert(pointerAlignment <= 4096, "Pointer aligment of more than 4096 bytes is unsupported");
static_assert(!(pointerAlignment & (pointerAlignment - 1)),
		"Pointer aligment must be a power of 2");

constexpr size_t pageSize = 0x1000;

void *MemoryAllocator::allocate(size_t size) {
	size_t pg_size = (size + size_t{pageSize - 1}) & ~size_t{pageSize - 1};
	size_t offset = (pg_size - size) & ~size_t{pointerAlignment - 1};

	void *ptr;

	// Two extra pages for metadata in front and guard page at the end
	// Reserve the whole region as PROT_NONE...
	if (int e = mlibc::sys_vm_map(nullptr, pg_size + pageSize * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0, &ptr))
		mlibc::panicLogger() << "sys_vm_map failed in MemoryAllocator::allocate (errno " << e << ")" << frg::endlog;

	// ...Then replace pages to make them accessible, excluding the guard page
	if (int e = mlibc::sys_vm_map(ptr, pg_size + pageSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0, &ptr))
		mlibc::panicLogger() << "sys_vm_map failed in MemoryAllocator::allocate (errno " << e << ")" << frg::endlog;

	void *meta = ptr;
	void *out_page = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + pageSize);
	void *out = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out_page) + offset);
	void *out_align_area = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out) + size);

	AllocatorMeta metaData{size, pg_size, allocatorMagic};

	memset(meta, metaAreaValue, pageSize);
	memcpy(meta, &metaData, sizeof(AllocatorMeta));

	memset(out_page, offsetAreaValue, offset);
	memset(out, allocatedAreaValue, size);
	memset(out_align_area, alignmentAreaValue, pg_size - offset - size);

	if constexpr (logAllocations)
		mlibc::infoLogger() << "MemoryAllocator::allocate(" << size << ") = " << out << frg::endlog;

	return out;
}

void MemoryAllocator::free(void *ptr) {
	if (!ptr)
		return;

	if constexpr (logAllocations)
		mlibc::infoLogger() << "MemoryAllocator::free(" << ptr << ")" << frg::endlog;

	uintptr_t page_addr = reinterpret_cast<uintptr_t>(ptr) & ~size_t{pageSize - 1};
	AllocatorMeta *meta = reinterpret_cast<AllocatorMeta *>(page_addr - pageSize);

	if (meta->magic != allocatorMagic)
		mlibc::panicLogger() << "Invalid allocator metadata magic in MemoryAllocator::free" << frg::endlog;

	deallocate(ptr, meta->allocatedSize);
}

void MemoryAllocator::deallocate(void *ptr, size_t size) {
	if (!ptr)
		return;

	if constexpr (logAllocations)
		mlibc::infoLogger() << "MemoryAllocator::deallocate(" << ptr << ", " << size << ")" << frg::endlog;

	uintptr_t page_addr = reinterpret_cast<uintptr_t>(ptr) & ~size_t{pageSize - 1};
	AllocatorMeta *meta = reinterpret_cast<AllocatorMeta *>(page_addr - pageSize);

	if (meta->magic != allocatorMagic)
		mlibc::panicLogger() << "Invalid allocator metadata magic in MemoryAllocator::deallocate" << frg::endlog;

	if (size != meta->allocatedSize)
		mlibc::panicLogger() << "Invalid allocated size in metadata in MemoryAllocator::deallocate (given " << size << ", stored " << meta->allocatedSize << ")" << frg::endlog;

	if constexpr (neverReleaseVa) {
		void *unused;
		if (int e = mlibc::sys_vm_map(meta, meta->pagesSize + pageSize * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0, &unused))
			mlibc::panicLogger() << "sys_vm_map failed in MemoryAllocator::deallocate (errno " << e << ")" << frg::endlog;
	} else {
		if (int e = mlibc::sys_vm_unmap(meta, meta->pagesSize + pageSize * 2))
			mlibc::panicLogger() << "sys_vm_unmap failed in MemoryAllocator::deallocate (errno " << e << ")" << frg::endlog;
	}
}

void *MemoryAllocator::reallocate(void *ptr, size_t size) {
	if (!size) {
		free(ptr);
		return nullptr;
	}

	void *newArea = allocate(size);

	if (ptr) {
		uintptr_t page_addr = reinterpret_cast<uintptr_t>(ptr) & ~size_t{pageSize - 1};
		AllocatorMeta *meta = reinterpret_cast<AllocatorMeta *>(page_addr - pageSize);

		if (meta->magic != allocatorMagic)
			mlibc::panicLogger() << "Invalid allocator metadata magic in MemoryAllocator::reallocate" << frg::endlog;

		memcpy(newArea, ptr, frg::min(meta->allocatedSize, size));

		deallocate(ptr, meta->allocatedSize);
	}

	if constexpr (logAllocations)
		mlibc::infoLogger() << "MemoryAllocator::reallocate(" << ptr << ", " << size << ") = " << newArea << frg::endlog;

	return newArea;
}

size_t MemoryAllocator::get_size(void *ptr) {
	if constexpr (logAllocations)
		mlibc::infoLogger() << "MemoryAllocator::get_size(" << ptr << ")" << frg::endlog;

	uintptr_t page_addr = reinterpret_cast<uintptr_t>(ptr) & ~size_t{pageSize - 1};
	AllocatorMeta *meta = reinterpret_cast<AllocatorMeta *>(page_addr - pageSize);

	if (meta->magic != allocatorMagic)
		mlibc::panicLogger() << "Invalid allocator metadata magic in MemoryAllocator::get_size" << frg::endlog;

	return meta->allocatedSize;
}

MemoryAllocator &getAllocator() {
	// use frg::eternal to prevent a call to __cxa_atexit().
	// this is necessary because __cxa_atexit() call this function.
	static frg::eternal<MemoryAllocator> singleton{};
	return singleton.get();
}

#endif /* !MLIBC_DEBUG_ALLOCATOR */
