#include "windows.h"
#include "DH_MacroAbuse.h"
#include "L:\Mem.cpp"
#include <vector>


DH_PlatformAllocator platform;
DH_GeneralPurposeAllocator gp(&platform);
DH_TrackingAllocator tracker(&gp);
DH_TrackingAllocatorUser subsystem1(&tracker, "sub_1");
DH_TrackingAllocatorUser subsystem2(&tracker, "sub_2");

int main()
{
	MemBlock blocks[200];
	for (int i = 1; i < 150; i++) {
		if(i&1)blocks[i] = subsystem1.alloc(i * 20);
		else   blocks[i] = subsystem2.alloc(i * 20);
		printf("ptr=%p\n", blocks[i].mem);
	}
	//a.free_all();
	for (int i = 1; i < 150; i++) {
		subsystem1.free(&blocks[i]);
	}

	int q = 0;
}