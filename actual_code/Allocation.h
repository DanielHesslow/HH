#ifndef Allocations
#define Allocations

void *alloc_(size_t memorySize, void *allocationInfoString, void *owner);
void *alloc_(size_t memorySize, void *allocationInfoString) { return alloc_(memorySize, allocationInfoString,(void *)0); }
void free_(void* memory, void *owner);
void free_(void *memory ) {free_(memory, (void *)0); }

struct Allocation
{
	char* start;
	size_t length;
	char *allocationInfo;
};

struct DH_Allocator
{
	void *(*alloc)(size_t bytes_to_alloc, void *allocation_info, void *data);
	void(*free)(void *mem_to_free, void *data);
	void *data;
};

void *Allocate(DH_Allocator allocator, size_t bytes_to_alloc, void *allocation_info)
{
	return allocator.alloc(bytes_to_alloc, allocation_info, allocator.data);
}

void DeAllocate(DH_Allocator allocator, void *mem)
{
	allocator.free(mem, allocator.data);
}

#endif