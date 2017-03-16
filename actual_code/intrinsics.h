#ifndef INTRINSICS_H
#define INTRINSICS_H

#ifdef _WIN32
#include "intrin.h"
#include "stdint.h"






bool BSR64(int *index, int64_t value) {
	unsigned long pindex;
	bool success = _BitScanReverse64(&pindex, (unsigned __int64)value);
	*index = pindex;
	return success;
}

bool BSF64(int *index, int64_t value) {
	unsigned long pindex;
	bool success = _BitScanForward64(&pindex, (unsigned __int64)value);
	*index = pindex;
	return success;
}


#else 

#endif
#endif
