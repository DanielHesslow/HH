#include"Mem.h"

#define static_error(message) static_assert(false,message)

#ifndef SWAP
	#define SWAP(type, a, b)do{type tmp = (a); (a) = (b); (b) = tmp;}while(0)
#endif

#include <allocators>

#ifndef TYPE
	static_error("TYPE macro must be defined (dynamic array)");
#endif


#ifndef NAME
	static_error("NAME macro must be defined (dynamic array)");
#endif

#ifndef GROWTH_RATE
	#define GROWTH_RATE 2
#endif


struct NAME
{
	#if STACK_SIZE
	TYPE stack_arr[STACK_SIZE];
	#endif

	
	DH_Allocator *allocator;
	union {
		MemBlock memory;
		TYPE *start;
	};
	int length;

	TYPE &operator[](int i) 
	{ 
	#ifndef NO_ABC
		assert(i >= 0 && i < length); 
	#endif
		return start[i]; 
	}

private:
#ifdef ORDER
	inline bool greater_than(TYPE a, TYPE b)
	{
		return ORDER(b,a);
	}

	inline bool smaller_than(TYPE a, TYPE b)
	{
		return ORDER(a,b);
	}
#endif
#ifdef EQUAL
	inline bool equal(TYPE a, TYPE b)
	{
		return EQUAL(a,b);
	}
#endif

#if STACK_SIZE
	bool is_using_static_arr() const {
		return memory.mem == stack_arr;
	}
#endif
public:
	NAME(DH_Allocator *allocator)
	{
		length = 0;
		this->allocator = allocator;
		memory.cap = 0;
		memory.mem = NULL;
		#if STACK_SIZE
			memory.mem = stack_arr;
			memory.cap = sizeof(stack_arr);
		#endif
	}
#if STACK_SIZE
	NAME(const NAME &other){
		allocator = other.allocator;
		memory = other.memory;
		length = other.length;
		if (other.is_using_static_arr()) {
			memory.mem = stack_arr;
			memmove(stack_arr,other.stack_arr,sizeof(stack_arr));
		}
	}
#endif

	

	void destroy() {
		allocator->free(&memory);
		length = 0;
	}

	void reserve(int new_num_elements)
	{
		if (new_num_elements * sizeof(TYPE) < memory.cap) return;
		int cap = max(memory.cap,16*sizeof(TYPE));
		do { cap *= GROWTH_RATE; } while (cap < new_num_elements * sizeof(TYPE));
		MemBlock block_copy = memory; // dance around because we can't invalidate our self in the gp allocator...
		#if STACK_SIZE
			MemBlock blk = allocator->alloc(cap);
			memmove(blk.mem, memory.mem, DH_MIN(blk.cap, memory.cap));
			if(memory.mem != stack_arr) allocator->free(&memory);
			memory = blk;
		#else
			#ifdef CLEAR
				allocator->realloc_and_zero(&block_copy, cap);
			#else
				allocator->realloc(&memory, cap);
			#endif
		#endif
	}

	void clear()
	{
		mem_block_clear(&memory);
		length = 0;
	}

#ifdef ORDER
	bool binary_search(TYPE elem, int *_out_index = 0)
	{
		if (length == 0)
		{
			*_out_index = 0;
			return false;
		}

		int low = 0;
		int high = length;
		for (;;)
		{
			int center = (low + high) / 2;
			if (low == center)
			{
				assert(low + 1 == high);
				bool gt = greater_than(elem, start[low]);
				if (_out_index)
					*_out_index = center + gt;
				return !(gt || smaller_than(elem, start[0]));
			}
			if (smaller_than(elem, start[center]))
			{
				high = center;
			}
			else
			{
				low = center;
			}
		}
	}

	void unsafe_insert(TYPE value, int index)
	{
		if (index < 0 || index>length)return;
		reserve(length+1);
		memmove(&start[index + 1], &start[index], sizeof(TYPE)*(length - index));
		start[index] = value;
		++length;
	}
#endif
#ifdef EQUAL	
	bool linear_search(TYPE elem, int *_out_index = 0)
	{
		for (int i = 0; i<length;i++)
		{
			if(equal(start[i], elem))
			{
				*_out_index = i;
				return true;
			}
		}

		*_out_index = 0;
		return false;
	}	
#endif
#ifndef ORDER
	void add(TYPE elem)
	{
		reserve(length+1);
		start[length++] = elem;
	}
#else 
	void add(TYPE elem)
	{
		int out_index;
		binary_search(elem, &out_index);
		unsafe_insert(elem, out_index);
	}
#endif

	void pop()
	{
		--length;
		#ifdef CLEAR
			memset(&start[length], 0, sizeof(TYPE));
		#endif
	}
#ifdef ORDER
	void add_unordered(TYPE elem)
	{
		reserve(length+1);
		start[length++] = elem;
	}

private:
	int qs_partition(int low, int high)
	{
		TYPE pivot = start[low];
		int i = low - 1;
		int j = high + 1;
		for (;;)
		{
			do { ++i; } while (smaller_than(start[i], pivot));
			do { --j; } while (greater_than(start[j], pivot));
			if (i >= j) return j;
			SWAP(TYPE, start[i], start[j]);
		}
	}
	
	
	void quick_sort(int low, int high)
	{
		if (high - low < 30) return;
		else
		{
			int p = qs_partition(low, high);
			quick_sort(low, p);
			quick_sort(p + 1, high);
		}
	}

	void insertion_sort(int low, int high)
	{
		for (int i = low+1; i <= high; i++)
		{
			TYPE tmp = start[i];
			int j = i - 1;
			while (j >= 0 && greater_than(start[j], tmp))
			{
				start[j + 1] = start[j]; //shift down values
				--j;
			}
			start[j + 1] = tmp; //insert value in new place.
		}
	}
public:
	void sort()
	{
		quick_sort(0, length-1);
		insertion_sort(0, length-1);
	}
	void near_sorted()
	{
		insertion_sort(0, length - 1);
	}
#endif


	void remove(int index)
	{
		if(index < 0 || index >= length ) 
		{
			assert(false && "tried to remove out of bounds");
			return; //nothing we can do.
		}
		#ifdef ORDER
		memmove(&start[index], &start[index+1], sizeof(TYPE)*(length - index - 1));
		--length;
		#else
		start[index] = start[--length];
		#endif
		#ifdef CLEAR
		start[length] =0;
		#endif
	}
};

#ifndef NO_UNDEF // if we wan't multiple arrays with same attriutes
	#ifdef CLEAR
		#undef CLEAR;
	#endif

	#ifdef TYPE
		#undef TYPE
	#endif

	#ifdef ORDER
		#undef ORDER
	#endif


	#ifdef EQUAL
		#undef EQUAL
	#endif

	#ifdef STACK_SIZE
		#undef STACK_SIZE
	#endif


	#ifdef NAME
		#undef NAME
	#endif

	#ifdef GROWTH_RATE
		#undef GROWTH_RATE
	#endif

	#ifdef NO_ABC
		#undef NO_ABC
	#endif
#endif


