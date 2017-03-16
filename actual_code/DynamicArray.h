#ifndef DA_SWAP
#define DA_SWAP(type, a, b)do{type tmp = (a); (a) = (b); (b) = tmp;}while(0)
#endif

#ifndef DA_TYPE
#error DA_TYPE macro must be defined 
#define DA_ERROR
#endif


#ifndef DA_NAME
#error DA_NAME macro must be defined
#define DA_ERROR
#endif

#ifndef DA_GROWTH_RATE
#define DA_GROWTH_RATE 2
#endif


#ifndef DA_ERROR
struct DA_NAME {
	DH_Allocator *allocator;
	union {
		MemBlock memory;
		struct {
			DA_TYPE *start;
		};
	};
	
	size_t capacity() {
		return memory.cap / sizeof(DA_TYPE);
	}
	int length;

	DA_TYPE &operator[](int i) {
#ifndef DA_NO_ABC
		assert(i >= 0 && i < length);
#endif
		return start[i];
	}

private:
#ifdef DA_ORDER
	inline bool greater_than(DA_TYPE a, DA_TYPE b) {
		return DA_ORDER(b, a);
	}

	inline bool smaller_than(DA_TYPE a, DA_TYPE b) {
		return DA_ORDER(a, b);
	}
#endif
#ifdef DA_EQUAL
	inline bool equal(DA_TYPE a, DA_TYPE b) {
		return DA_EQUAL(a, b);
	}
#endif

public:
	static DA_NAME make(DH_Allocator *allocator) {
		DA_NAME ret = {};
		ret.length = 0;
		ret.allocator = allocator;
		ret.length = 0;
		ret.memory.cap = 0;
		ret.memory.mem = NULL;
		return ret;
	}

	void destroy() {
		allocator->free(&memory);
		length = 0;
	}

	void reserve(int new_num_elements) {
		if (new_num_elements * sizeof(DA_TYPE) < memory.cap) return;
		int cap = max(memory.cap, 16 * sizeof(DA_TYPE));
		do { cap *= DA_GROWTH_RATE; } while (cap < new_num_elements * sizeof(DA_TYPE));
		MemBlock block_copy = memory; // dance around because we can't invalidate our self in the gp allocator...
		allocator->realloc(&memory, cap);
	}

	void clear() {
		mem_block_clear(&memory);
		length = 0;
	}

#ifdef DA_ORDER
	bool binary_search(DA_TYPE elem, int *_out_index = 0) {
		if (length == 0) {
			*_out_index = 0;
			return false;
		}

		int low = 0;
		int high = length;
		for (;;) {
			int center = (low + high) / 2;
			if (low == center) {
				assert(low + 1 == high);
				bool gt = greater_than(elem, start[low]);
				if (_out_index)
					*_out_index = center + gt;
				return !(gt || smaller_than(elem, start[0]));
			}
			if (smaller_than(elem, start[center])) {
				high = center;
			} else {
				low = center;
			}
		}
	}

	void unsafe_insert(DA_TYPE value, int index) {
#ifndef DA_NO_ABC
		if (index < 0 || index>length)return;
#endif
		reserve(length + 1);
		memmove(&start[index + 1], &start[index], sizeof(DA_TYPE)*(length - index));
		start[index] = value;
		++length;
	}
#else
	void insert(DA_TYPE value, int index) {
#ifndef DA_NO_ABC
		if (index < 0 || index>length)return;
#endif
		reserve(length + 1);
		memmove(&start[index + 1], &start[index], sizeof(DA_TYPE)*(length - index));
		start[index] = value;
		++length;
	}
#endif
#ifdef DA_EQUAL	
	bool linear_search(DA_TYPE elem, int *_out_index = 0) {
		for (int i = 0; i < length; i++) {
			if (equal(start[i], elem)) {
				*_out_index = i;
				return true;
			}
		}

		*_out_index = 0;
		return false;
	}
#endif
#ifndef DA_ORDER
	void add(DA_TYPE elem) {
		reserve(length + 1);
		start[length++] = elem;
	}
#else 
	void insert(DA_TYPE elem) {
		int out_index;
		binary_search(elem, &out_index);
		unsafe_insert(elem, out_index);
	}
#endif

#ifdef DA_ORDER
	void add_unordered(DA_TYPE elem) {
		reserve(length + 1);
		start[length++] = elem;
	}

private:
	int qs_partition(int low, int high) {
		DA_TYPE pivot = start[low];
		int i = low - 1;
		int j = high + 1;
		for (;;) {
			do { ++i; } while (smaller_than(start[i], pivot));
			do { --j; } while (greater_than(start[j], pivot));
			if (i >= j) return j;
			DA_SWAP(DA_TYPE, start[i], start[j]);
		}
	}


	void quick_sort(int low, int high) {
		if (high - low < 30) return;
		else {
			int p = qs_partition(low, high);
			quick_sort(low, p);
			quick_sort(p + 1, high);
		}
	}

	void insertion_sort(int low, int high) {
		for (int i = low + 1; i <= high; i++) {
			DA_TYPE tmp = start[i];
			int j = i - 1;
			while (j >= 0 && greater_than(start[j], tmp)) {
				start[j + 1] = start[j]; //shift down values
				--j;
			}
			start[j + 1] = tmp; //insert value in new place.
		}
	}
public:
	void sort() {
		quick_sort(0, length - 1);
		insertion_sort(0, length - 1);
	}
	void near_sorted() {
		insertion_sort(0, length - 1);
	}
#endif

	void bounds_check_remove(int index) {
		#ifndef DA_NO_ABC
		if (index < 0 || index >= length) {
			assert(false && "tried to remove out of bounds");
			return; //nothing we can do.
		}
		#endif
	}

#ifdef DA_ORDER
	void remove(int index) {
		bounds_check_remove(index);
		memmove(&start[index], &start[index + 1], sizeof(DA_TYPE)*(length - index - 1));
		--length;
	}
#else
	void remove(int index) {
		bounds_check_remove(index);
		start[index] = start[--length];
	}
	void removeOrd(int index) {
		bounds_check_remove(index);
		memmove(&start[index], &start[index + 1], sizeof(DA_TYPE)*(length - index - 1));
		--length;
	}
#endif

};
#endif

#undef DA_ERROR
#undef DA_NAME
#undef DA_TYPE
#undef DA_ORDER
#undef DA_EQUAL
#undef DA_GROWTH_RATE
#undef DA_NO_ABC


