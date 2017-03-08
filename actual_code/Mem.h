#pragma once
#include "string.h"
#include "stdint.h"
#include "intrinsics.h"

struct MemBlock {
	void  *mem;
	size_t cap;
	void *end() {
		return (char *)mem + cap;
	}
	
	bool owns(void *memory) {
		char *m_start = (char *)mem;
		char *m_end = (char *)end();
		return (char *)memory >= m_start && (char *)memory <= m_end;
	}

	bool owns(MemBlock block) {
		return owns(block.mem) && owns(block.end());
	}
};

void mem_block_clear(MemBlock *memory_block) {
	if (memory_block) memset(memory_block->mem, 0, memory_block->cap);
}

#define DH_MIN(a,b) (a<b?a:b)
class DH_Allocator {
public:
	virtual MemBlock alloc(size_t num_bytes) = 0;
	virtual void	 free(MemBlock *mem_block) = 0;
	virtual void	 realloc(MemBlock *mem_block, size_t new_num_bytes) {
		MemBlock blk = alloc(new_num_bytes);
		memmove(blk.mem, mem_block->mem, DH_MIN(blk.cap, mem_block->cap));
		free(mem_block);
		*mem_block = blk;
	}
	MemBlock alloc_and_zero(size_t num_bytes) {
		MemBlock blk = alloc(num_bytes);
		mem_block_clear(&blk);
		return blk;
	}

	void realloc_and_zero(MemBlock *memory_block, size_t new_num_bytes) {
		size_t prev_cap = memory_block->cap;
		realloc(memory_block, new_num_bytes);
		memset((char *)memory_block->mem + prev_cap, 0, memory_block->cap - prev_cap);
	}
};

#define TYPE MemBlock
#define NAME DA_MemBlock
#include "DynamicArray.h"

struct DH_GeneralPurposeAllocator;
//this is the constant info that can be computed by parent so no need to store it.
//this only depend on the buckets index.
struct BucketInfo {
	DH_GeneralPurposeAllocator *internal_allocator;
	DH_Allocator *chunk_allocator;
	size_t chunk_size;
	size_t elem_size;
	int	elems_per_chunk;
};

struct Bucket {
	MemBlock memory;
	int      num_chunks;
	int      cap_chunks;

	uint64_t *get_bit_field();
	void **get_chunks(BucketInfo *bucket_info);
	MemBlock get_elem(BucketInfo * bucket_info, int index);
	void restore_invariant(BucketInfo * bucket_info);
	MemBlock aquire_elem(BucketInfo * bucket_info);
	MemBlock get_chunk(BucketInfo *bucket_info, int index);
	void free_elem(BucketInfo * bucket_info, MemBlock *mem_block);
	void reallocate(BucketInfo *bucket_info, size_t new_cap);
};

class DH_GeneralPurposeAllocator : public DH_Allocator {
public:
	DH_Allocator *parent;
	DH_GeneralPurposeAllocator(DH_Allocator *parent);
	Bucket buckets[26]; // goes up to two gigs sizes. plenty enough
	MemBlock alloc(size_t num_bytes);
	MemBlock alloc(size_t num_bytes, bool internal, Bucket **used_bucket, BucketInfo *used_bucket_info);
	void	 free(MemBlock *mem_block);
};

class DH_PlatformAllocator : public DH_Allocator {
public:
	MemBlock alloc(size_t num_bytes);
	void	 free(MemBlock *mem_block);
};


#define NAME DA_ORD_MemBlock
#define TYPE MemBlock
#define ORDER(a,b) a.mem < b.mem
#define EQUAL(a,b) a.mem == b.mem
#include "DynamicArray.h"

class DH_SlowArena :public DH_Allocator {
public:
	DH_Allocator *parent;
	DH_SlowArena(DH_Allocator *parent);
	DA_ORD_MemBlock allocations;
	MemBlock alloc(size_t num_bytes);
	void	 free(MemBlock *mem_block);
	void     free_all();
};


struct MemBlockInfo {
	char *info;
	size_t insertion_index;
	MemBlock block;
};

#define NAME DA_MemBlockInfo
#define TYPE MemBlockInfo
#define ORDER(a,b) a.insertion_index <  b.insertion_index
#define EQUAL(a,b) a.block.mem == b.block.mem
#include "DynamicArray.h"

class DH_TrackingAllocator :public DH_Allocator {
public:
	DH_Allocator *parent;
	size_t insertion_index;
	DH_TrackingAllocator(DH_Allocator *parent);
	DA_MemBlockInfo allocations;
	MemBlock alloc(size_t num_bytes);
	MemBlock alloc(size_t num_bytes, char *info);
	void	 free(MemBlock *mem_block);
	void     free_all();
	void	 realloc(MemBlock *mem_block, size_t new_num_bytes);
};

class DH_TrackingAllocatorUser : public DH_Allocator {
public:
	char *info;
	DH_TrackingAllocator *parent;
	DH_TrackingAllocatorUser(DH_TrackingAllocator *parent, char *info);
	MemBlock alloc(size_t num_bytes);
	void	 free(MemBlock *mem_block);
};








