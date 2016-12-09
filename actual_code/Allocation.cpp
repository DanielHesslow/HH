#include "header.h"
#include "Allocation.h"
// this is a quick(to write) and dirty
// allocation strategy for debugging purposes. only.
// this will frament our memory a shit ton yoo..
// if we ever make this production quality we should have a round up to multiple of x bytes is used by our process. (for simd reads )
// and an alignement as well. call would look like

// alloc(size_t size, char *allocationInfo, int allign=4, int mult_padd_same_process=1)

global_variable char *memChunk;

DEFINE_DynamicArray(Allocation);
global_variable DynamicArray_Allocation blocks;



#if 1

void free_(void* memory);

char *blockEnd(Allocation block)
{
	return block.start + block.length;
}



void *alloc_(size_t memorySize, void *allocationInfo, void *owner) 
{
	memorySize *= 2;
	if (blocks.capacity <= blocks.length)
	{
		{	//we're not important. No need to bootstrap and stuff for a debug thing... Not yet anyway. 
			blocks.capacity += 1024*1024;									
			void *p = malloc(sizeof(Allocation)*(blocks.length+ 1024 * 1024));
			memcpy(p, blocks.start, (blocks.length)*sizeof(Allocation));				
			if (blocks.start)												
				free(blocks.start); //fail every time... donno why..
			blocks.start = (Allocation *)p;
		}
	}
	
	if (!memChunk) {
		size_t mem_amount = 1024 * 1024 * 100;//100 megs is a decent start, (we don't want any unnneccssary bloat)
		memChunk = (char *)VirtualAlloc(0, mem_amount, MEM_COMMIT, PAGE_READWRITE);
	}
	
	if (memorySize == 0)return ((void *)0);
	
	for (int i = 1; i < blocks.length; i++)
	{
		char *prevBlockEnd = blockEnd(blocks.start[i - 1]);
		char *nextBlockStart = blocks.start[i].start;
		size_t diff = nextBlockStart - prevBlockEnd;
		if (diff > memorySize)
		{
			Allocation block = {};
			block.start = prevBlockEnd+1;
			block.length = memorySize;
			block.allocationInfo = (char *)allocationInfo;
			Insert(&blocks, block,i); //why not -1 here?
			return block.start;
		}
	}

	Allocation block = {};
	if (blocks.length > 0) 
	{
		block.start = blockEnd(blocks.start[blocks.length - 1]);
		if ((block.start - memChunk)>=(1024 * 1024 * 100))
		{
			assert("exceded memmory allowence"&&false);
		}
	}
	else
		block.start = memChunk;
	block.length = memorySize;
	block.allocationInfo = (char *)allocationInfo;
	Add(&blocks, block);

	return block.start;
}

int getBlockIndex(void *memory)
{
	for (int i = 0; i < blocks.length; i++)
	{
		if (blocks.start[i].start == memory)
		{
			return i;
		}
	}
	return -1;
}

void searchAndFree(void *memory)
{
	if (memory == 0) return;
	int index = getBlockIndex(memory);
	if (index != -1)
		RemoveOrd(&blocks,index);
	else
		assert("freeing non-allocated memory " && false);
}

void free_(void* memory, void *owner)
{
	searchAndFree(memory);
}

#else
void free_(void* memory, void *owner)
{
	free(memory);
}

void *alloc_(size_t memorySize, const char*allocationInfo, void *owner)
{
	return malloc(memorySize);
}
#endif 