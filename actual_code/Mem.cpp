#include "Mem.h"
#include "intrinsics.h"
#include "DH_MacroAbuse.h"

#define MEM_ALIGN 16


// bitfield methods

bool bit_field_first_set(uint64_t *bit_field, int len, int *index) {
	int num_64 = (len + 63) / 64;
	for (int i = 0; i < num_64; i++) {
		int idx = 0;
		if (BSF64(&idx, bit_field[i])) {
			if (idx + 64 * i < len) {
				*index = idx + 64 * i;
				return true;
			} else {
				return false;
			}
		}
	}
	return false;
}

bool bit_field_first_clear(uint64_t *bit_field, int len, int *index) {
	int num_64 = (len + 63) / 64;
	for (int i = 0; i < num_64; i++) {
		int idx = 0;
		if (BSF64(&idx, ~bit_field[i])) {
			if (idx + 64 * i < len) {
				*index = idx + 64 * i;
				return true;
			} else {
				return false;
			}
		}
	}
	return false;
}

void bit_field_set_bit(uint64_t *bit_field, int len, int index) {
	assert(index < len && index >= 0);
	bit_field[index / 64] |= 1ull << (index % 64ull);
}

void bit_field_clear_bit(uint64_t *bit_field, int len, int index) {
	assert(index < len && index >= 0);
	bit_field[index / 64] &= ~(1ull << (index % 64ull));
}

bool bit_field_get_bit(uint64_t *bit_field, int len, int index) {
	assert(index < len && index >= 0);
	return bit_field[index / 64] & (1ull << (index % 64ull));
}

int get_index(size_t num_bytes) {
	int i1, i2;
	bool s = BSR64(&i1, num_bytes - 1) & BSR64(&i2, MEM_ALIGN); // rounds down, no good.
	assert(s);
	return max((i1 + 1) - i2, 0);
}



// General Purpose allocator information:
// The allocator uses an external alloacator to allocate it's chunks, probably a platform allocator like VirtualAlloc
// it never releases them!
// It allocates its internal datastructures recursively. 
// This is a little bit tricky to get right but can be made to work as long every bucket allways have space for *two* allocations
// without needing to resize. This will be refered to as the invariant.

// Why does this make sure that it works?
// an external party tries to allocate memory. Now there's only one free allocation left
// We then must try to restore the invariant. Which means that we need to reallocate our internal data structure.
// this call may go to ourself! thus the need to have the extra free allocation
// To make this work we must allways grow by atleast 2 elements to make sure we dont get stuck into a inf loop


// Here's the algorithm:
//
// External Allocation:
//   * acquire memory  
//   * restore invariant
//   * return memory to outside party

// Internal Allocation
//   * acquire memory
//   * update internal datastructures with the new memory
//   * restore invariant 


// Im pretty sure we could get this down to one extra but the gain is so tiny. and we'd need to restructure a bunch.

#define MIN_CHUNK_SIZE 4096

size_t element_size(int bucket_index) {
	return 16 << bucket_index;
}

BucketInfo bucket_info_from_index(DH_GeneralPurposeAllocator *allocator, int i) {
	assert(i < ARRAY_LENGTH(allocator->buckets) && "alloactions this big are currently not supported,"
		"increase number of buckets an gp allocator!");
	BucketInfo info = {};
	info.chunk_allocator = allocator->parent;
	info.elem_size = element_size(i);
	info.chunk_size = max(MIN_CHUNK_SIZE, info.elem_size * 4);
	info.internal_allocator = allocator;
	info.elems_per_chunk = info.chunk_size / info.elem_size;
	return info;
}

DH_GeneralPurposeAllocator::DH_GeneralPurposeAllocator(DH_Allocator *parent) {
	this->parent = parent;
	// This will initially overallocate a bunch
	// the idea is to bootstrap the allocation so to speak.
	// first we say that all buckets need the same size for it's internal memory
	// we first use the bucket of said size to allocate its *own* data structures
	// then we go on to allocate the others using the already set up bucket
	// at that point we have a working memory allocator. So we can just reallocate the buckets to the propper size.
	// we bootstrap the allocation so to speak :)

	// @NOTE we know that the first bucket will take the most space for it's internal data 
	//       as long as the chunk size won't grow fastrer than the element size
	//       which would not make any sence at all. (right?)

	// @NOTE maybe iterate to get this automatically?
	int predicted_num_chunks_needed = 1; //@NOTE NEED TUNING if CHUNK_SIZE OF ARRAY_LENGTH(buckets) changes, a check is done below to see if is correct

	int initial_chunk_cap = predicted_num_chunks_needed + 2;  // second two is for the invariant! do not touch!
	size_t max_bytes_bf = ((initial_chunk_cap * MIN_CHUNK_SIZE / 16 + 63) / 64) * sizeof(uint64_t);
	size_t bytes_chunks = initial_chunk_cap * sizeof(void *);
	size_t max_bytes_needed = max_bytes_bf + bytes_chunks;

	int bucket_index = get_index(max_bytes_needed);
	BucketInfo info = bucket_info_from_index(this, bucket_index);

	//check that the invariant holds afterwards
	int num_chunks_actually_needed = (max_bytes_needed * ARRAY_LENGTH(buckets) + info.chunk_size - 1) / info.chunk_size;
	assert(num_chunks_actually_needed <= initial_chunk_cap - 2); // if we trigger update predicted_num_chunks_needed


	MemBlock block;
	buckets[bucket_index].memory = { info.chunk_allocator->alloc_and_zero(info.chunk_size).mem, max_bytes_needed };
	buckets[bucket_index].cap_chunks = initial_chunk_cap;
	buckets[bucket_index].num_chunks = 0;

	buckets[bucket_index].get_chunks(&info)[buckets[bucket_index].num_chunks++] = buckets[bucket_index].memory.mem;

	MemBlock blk = buckets[bucket_index].aquire_elem(&info);
	assert(blk.mem == buckets[bucket_index].memory.mem);
	buckets[bucket_index].memory = blk; //sets the correct cap (not really needed)

	// at this point our memory allocator can handle calls that will go the bucket with index [bucket_index]

	for (int i = 0; i < ARRAY_LENGTH(buckets); i++) {
		if (i == bucket_index) continue;
		buckets[i].cap_chunks = initial_chunk_cap;
		buckets[i].num_chunks = 0;
		buckets[i].memory = buckets[bucket_index].aquire_elem(&info);
		mem_block_clear(&buckets[i].memory);
	}

	// now our memory allocator is finally fully functional!

	//so we can reallocate the buckets to a prefered size (larger than 2 though! so that the invariant holds)

	for (int i = 0; i < ARRAY_LENGTH(buckets); i++) {
		BucketInfo info = bucket_info_from_index(this, i);
		buckets[i].reallocate(&info, 8);
	}

}

void DH_GeneralPurposeAllocator::free(MemBlock *mem_block) {
	if (mem_block->mem == 0 && mem_block->cap == 0)return;
	int bucket_index = get_index(mem_block->cap);
	BucketInfo info = bucket_info_from_index(this, bucket_index);
	buckets[bucket_index].free_elem(&info, mem_block);
	*mem_block = {};
}



MemBlock DH_GeneralPurposeAllocator::alloc(size_t num_bytes, bool internal, Bucket **used_bucket, BucketInfo *used_bucket_info) {
	if (num_bytes == 0) return{};

	int bucket_index = get_index(num_bytes);
	BucketInfo info = bucket_info_from_index(this, bucket_index);
	MemBlock ret = buckets[bucket_index].aquire_elem(&info);
	if (!internal) {
		buckets[bucket_index].restore_invariant(&info);
	} else {
		*used_bucket = &buckets[bucket_index];
		*used_bucket_info = info;
	}
	return ret;
}

MemBlock DH_GeneralPurposeAllocator::alloc(size_t num_bytes) {
	return this->alloc(num_bytes, false, 0, 0);
}

uint64_t Bucket::get_bit_field() {
	return (uint64_t *)memory.mem;
}

void **Bucket::get_chunks(BucketInfo *bucket_info) {
	return (void **)((uint64_t *)memory.mem + (bucket_info->elems_per_chunk*cap_chunks + 63) / 64);
}

MemBlock Bucket::get_elem(BucketInfo * bucket_info, int index) {
	//@NOTE @PERF  is pow 2
	int chunk_index = index / bucket_info->elems_per_chunk;
	int sub_index = index % bucket_info->elems_per_chunk;
	MemBlock ret;
	ret.cap = bucket_info->elem_size;
	ret.mem = ((char *)(get_chunks(bucket_info)[chunk_index])) + sub_index * bucket_info->elem_size;
	return ret;
}

void Bucket::reallocate(BucketInfo *bucket_info, size_t new_cap) {
	size_t bf_size = ((new_cap * bucket_info->elems_per_chunk + 63) / 64) * 8;
	size_t chunk_size = new_cap * sizeof(void *);

	void **old_chunks = get_chunks(bucket_info);
	Bucket *used_bucket;
	BucketInfo used_bucket_info;
	MemBlock old_mem = memory;
	MemBlock new_mem = bucket_info->internal_allocator->alloc(chunk_size + bf_size, true, &used_bucket, &used_bucket_info);
	mem_block_clear(&new_mem);

	//move over the bitfield
	memmove(new_mem.mem, memory.mem, sizeof(uint64_t)*((bucket_info->elems_per_chunk*cap_chunks + 63) / 64));

	cap_chunks = new_cap;
	memory = new_mem;
	//move over the chunk array
	memmove(get_chunks(bucket_info), old_chunks, num_chunks * sizeof(void *));

	bucket_info->internal_allocator->free(&old_mem);

	//restore others invariant.
	used_bucket->restore_invariant(&used_bucket_info);
}

void Bucket::restore_invariant(BucketInfo *bucket_info) {

	// could instead search the bitfield.. but really not much to gain
	// this is little faster but allocates one extra chunk a little early. Eh.
	if ((cap_chunks - num_chunks) * bucket_info->elems_per_chunk < 2) {
		reallocate(bucket_info, cap_chunks * 2);
	}
}

MemBlock Bucket::aquire_elem(BucketInfo *bucket_info) {
	int index;
	if (!bit_field_first_clear(get_bit_field(), num_chunks*bucket_info->elems_per_chunk, &index)) {
		assert(num_chunks < cap_chunks);
		get_chunks(bucket_info)[num_chunks++] = bucket_info->chunk_allocator->alloc(bucket_info->chunk_size).mem;
		if (!bit_field_first_clear(get_bit_field(), num_chunks*bucket_info->elems_per_chunk, &index)) assert(false);
	}
	if (bit_field_get_bit(get_bit_field(), num_chunks*bucket_info->elems_per_chunk, index)) assert(false);
	bit_field_set_bit(get_bit_field(), num_chunks*bucket_info->elems_per_chunk, index);
	return get_elem(bucket_info, index);
}

MemBlock Bucket::get_chunk(BucketInfo *bucket_info, int index) {
	MemBlock ret;
	ret.cap = bucket_info->chunk_size;
	ret.mem = get_chunks(bucket_info)[index];
	return ret;
}

void Bucket::free_elem(BucketInfo * bucket_info, MemBlock *mem_block) {
	for (int i = 0; i < num_chunks; i++) {
		MemBlock chunk = get_chunk(bucket_info, i);
		if (chunk.owns(*mem_block)) {
			int bit_array_index = i*bucket_info->elems_per_chunk + ((char *)mem_block->mem - (char *)chunk.mem) / bucket_info->elem_size;
			assert(bit_field_get_bit(get_bit_field(), num_chunks*bucket_info->elems_per_chunk, bit_array_index) && "freeing twice (probably)");
			bit_field_clear_bit(get_bit_field(), num_chunks*bucket_info->elems_per_chunk, bit_array_index);
			return;
		}
	}
	assert(false && "freeing non-allocated element");
}



//PLATFORM ALLOCATOR

#include "windows.h"

void DH_PlatformAllocator::free(MemBlock *mem_block) {
	VirtualFree(mem_block->mem, mem_block->cap, MEM_COMMIT | MEM_RESERVE);
	*mem_block = {};
}

MemBlock DH_PlatformAllocator::alloc(size_t num_bytes) {
	void *mem = VirtualAlloc(NULL, num_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	MemBlock ret = { mem,num_bytes };
	return ret;
}

//Slow Arena ALLOCATOR 
// mostly a placeholder until I add a propper one!
#include "windows.h"
DH_SlowArena::DH_SlowArena(DH_Allocator *allocator) {
	allocations = DA_ORD_MemBlock::make(allocator);
	parent = allocator;
}

void DH_SlowArena::free(MemBlock *mem_block) {
	if (mem_block->cap == 0 && mem_block->mem == 0)return;
	int index;
	allocations.binary_search(*mem_block, &index);
	allocations.remove(index);
	parent->free(mem_block);
}

MemBlock DH_SlowArena::alloc(size_t num_bytes) {
	MemBlock new_mem = parent->alloc(num_bytes);
	allocations.add(new_mem);
	return new_mem;
}

void DH_SlowArena::free_all() {
	int index;
	for (int i = 0; i < allocations.length; i++) {
		parent->free(&allocations[i]);
	}
	allocations.destroy();
}


// --- tracking arena

DH_TrackingAllocator::DH_TrackingAllocator(DH_Allocator *allocator){
	allocations = DA_ORD_MemBlock::make(allocator);
	parent = allocator;
	total_num_allocations = 0;
}

MemBlock DH_TrackingAllocator::alloc(size_t num_bytes, char *allocation_info) {
	MemBlock blk = parent->alloc(num_bytes);
	MemBlockInfo info = { allocation_info,total_num_allocations++, blk };
	allocations.add(info);
	return blk;
}


MemBlock DH_TrackingAllocator::alloc(size_t num_bytes) {
	return alloc(num_bytes, "no_info_set");
}

void DH_TrackingAllocator::free(MemBlock *mem_block) {
	if (mem_block->cap == 0 && mem_block->mem == 0)return;
	int index;
	MemBlockInfo info = { 0,0,*mem_block };
	allocations.linear_search(info, &index);
	allocations.remove(index);
	parent->free(mem_block);
}

void DH_TrackingAllocator::free_all() {
	int index;
	for (int i = 0; i < allocations.length; i++) {
		parent->free(&allocations[i].block);
	}
	allocations.destroy();
}

void DH_TrackingAllocator::realloc(MemBlock *mem_block, size_t new_num_bytes) {
	MemBlockInfo info = { 0,0,*mem_block };
	int index;
	bool s = allocations.linear_search(info, &index);
	if (s) info = allocations[index];

	MemBlock blk = alloc(new_num_bytes, info.info);
	memmove(blk.mem, mem_block->mem, DH_MIN(blk.cap, mem_block->cap));

	//free
	if (mem_block->mem != 0 && mem_block->cap != 0) {
		allocations.remove(index);
		parent->free(mem_block);
	} else {
		assert(s);
	}
	*mem_block = blk;
}


// tracking Allocator user

DH_TrackingAllocatorUser::DH_TrackingAllocatorUser(DH_TrackingAllocator *parent, char *info) {
	this->info = info;
	this->parent = parent;
}


MemBlock DH_TrackingAllocatorUser::alloc(size_t num_bytes) {
	return parent->alloc(num_bytes, info);
}

void DH_TrackingAllocatorUser::free(MemBlock *mem_block) {
	parent->free(mem_block);
}




