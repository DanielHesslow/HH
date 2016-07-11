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


#endif