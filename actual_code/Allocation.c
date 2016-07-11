#include "Allocation.h"
void *alloc_(size_t memorySize);
void free_(void* memory);

struct Block
{
	void* start;
	size_t length;
};
DEFINE_DynamicArray(Block)
DynamicArray_Block memoryBlocs = constructDynamicArray_Block(200);
void *alloc_(size_t memorySize)
{
	return malloc(memorySize);
}
void free_(void* memory)
{
	free(memory);
}
