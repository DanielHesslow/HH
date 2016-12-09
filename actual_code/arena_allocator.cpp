#include "DH_DataStructures.h"

#define ALLOCATION_GRANULARITY_IN_BYTES 4

struct AllocationBlock
{
	char *start;
	size_t size;
	char *allocationInfo;
};

struct Arena
{
	size_t size;
	size_t allocations;
	Arena *next;
	DH_Allocator allocator;
	char first_item;
};

char *ciel_ptr(char *p)
{
	return (char *)ciel_to_multiple_of((uint64_t) p, ALLOCATION_GRANULARITY_IN_BYTES);
}

AllocationBlock *allocation_block_from_index(Arena *arena, int index)
{
	return &(((AllocationBlock *)(((char *)arena) + arena->size))[-index-1]);
}
char *last_block_end(Arena *arena)
{
	if (arena->allocations == 0) return &arena->first_item;
	AllocationBlock *block = allocation_block_from_index(arena, arena->allocations - 1);
	return block->start + block->size; 
}

bool can_push(Arena *arena, size_t bytes)
{
	return (char *)allocation_block_from_index(arena, arena->allocations) > ciel_ptr(last_block_end(arena))+bytes;
}
struct ArenaIterator
{
	Arena *arena;
	int element_index;
};

bool next(ArenaIterator *iterator, AllocationBlock **_out_element);
ArenaIterator make_iterator(Arena *arena);

void validate_arena(Arena *arena)
{
	//check overflows...
	char *start = last_block_end(arena);
	char *end = (char *)allocation_block_from_index(arena, arena->allocations - 1);
	while (start != end)
	{
		assert(*(unsigned char *)start == 0xef);
		++start;
	}

	if (arena->allocations == 0)return;
	auto it = make_iterator(arena);
	AllocationBlock *block;
	
	AllocationBlock *last;
	next(&it, &last);
	while (next(&it, &block))
	{
		assert(last->start + last->size <= block->start);
		last = block;
	}
	assert(last->start + last->size == last_block_end(it.arena));
	assert((char *)allocation_block_from_index(arena,arena->allocations-1) >= last_block_end(arena));
}

AllocationBlock *push(Arena *arena, size_t bytes, char *allocationInfo)
{
	validate_arena(arena);
	AllocationBlock *block = allocation_block_from_index(arena, arena->allocations);
	block->allocationInfo = allocationInfo;
	block->size = bytes;
	block->start = last_block_end(arena);
	++arena->allocations;
	validate_arena(arena);
	return block;
}

Arena *allocateArena(size_t size, DH_Allocator allocator, char *allocation_info)
{
	Arena *arena= (Arena *)Allocate(allocator, size, allocation_info);
	memset(arena, 0xEF, size);
	arena->allocator = allocator;
	arena->allocations = 0;
	arena->next = (Arena *)0;
	arena->size= size;
	return arena;
}
DH_Allocator arena_allocator(Arena *arena);



bool next(ArenaIterator *iterator, AllocationBlock **_out_element)
{
	if (iterator->element_index + 1 > iterator->arena->allocations)
	{
		if (iterator->arena->next)
		{
			iterator->arena = iterator->arena->next;
			iterator->element_index = 0;
		}
		else
		{
			return false;
		}
	}
	if(_out_element)
		*_out_element = allocation_block_from_index(iterator->arena, iterator->element_index++);
	return true;
}

ArenaIterator make_iterator(Arena *arena)
{
	ArenaIterator it = {};
	it.arena = arena;
	return it;
}

void *arena_alloc(size_t bytes_to_alloc, void *allocation_info, Arena *arena)
{
	assert(bytes_to_alloc < arena->size); // we need to handle this aswell though


	if (can_push(arena, bytes_to_alloc))
	{
		AllocationBlock *block = push(arena, bytes_to_alloc, (char *)allocation_info);
		return block->start;
	}
	else
	{ //squeeeze things in the middle or free list or somehting?
		if (!arena->next)
		{
			arena->next = allocateArena(arena->size, arena->allocator, (char *)allocation_info);
		}
		return arena_alloc(bytes_to_alloc, allocation_info, arena->next);
	}
}

void arena_delloc(void *mem_to_free, Arena *arena)
{
	ArenaIterator it = make_iterator(arena);
	AllocationBlock *current;
	while(next(&it,&current))
	{
		if ((void *)current->start == mem_to_free)
		{	
			// this remove needs to be ordered yooo...
			AllocationBlock *lastBlock = allocation_block_from_index(it.arena, --it.arena->allocations);;
			size_t move_len = (current- lastBlock) * sizeof(AllocationBlock);
			memset(current->start, 0xef, current->size);
			memmove(lastBlock + 1, lastBlock, move_len);
			memset(lastBlock, 0xef, sizeof(AllocationBlock));
			return;
		}
	}
	if(mem_to_free)
		assert(false&&"freeing nonallocated memory");
}

DH_Allocator arena_allocator(Arena *arena)
{
	dprs("new arena");
	DH_Allocator ret = {};
	ret.data = arena;
	ret.alloc = [](size_t bytes_to_alloc, void *allocation_info, void *arena) {return arena_alloc(bytes_to_alloc, allocation_info, (Arena *)arena); };
	ret.free = [](void *mem, void *data) {arena_delloc(mem, (Arena *)data); };
	return ret;
}

void arena_deallocate_all(DH_Allocator allocator) //this assumes it's a underlying arena and doesn't do any checking which probably is a bad idea or somehting
{
	Arena *arena = (Arena *)allocator.data;
	do {
		Arena *next = arena->next;
		DeAllocate(arena->allocator, arena);
		arena = next;
	} while (arena);
}

void arena_clear(DH_Allocator allocator)
{
	Arena *arena = (Arena *)allocator.data;
	arena->allocations = 0;

	Arena *next_arena = arena->next;
	while(next_arena)
	{
		Arena *next = arena->next;
		DeAllocate(arena->allocator, arena);
		next_arena = next;
	}
	size_t size = arena->size;
	memset(arena, 0xEF, size);
	arena->allocations = 0;
	arena->next = 0;
	arena->size= size;
}



