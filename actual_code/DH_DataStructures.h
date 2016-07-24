#ifndef DH_DataStructures
#define DH_DataStructures




#define DynamicArray_StructMeat(type)	\
	int length;							\
	int capacity;						\
	type *start;						\
	void *allocationInfo;				\
	DH_Allocator allocator;				\

// --- DYNAMICARRAYS
// insert is allowed to be in the range 0 arr->length
// insert(&arr, arr->length) does the same thing as add(&arr);

struct DynamicArray_void
{
	DynamicArray_StructMeat(void)
};

#define DA_GROWTHRATE 1.5
void _grow_to(DynamicArray_void *arr, size_t type_size, int new_capacity)
{
	//realloc_(&arr->start,type_size*arr->capacity);
	void *new_mem = Allocate(arr->allocator,new_capacity * type_size, arr->allocationInfo);
	memmove(new_mem, arr->start, arr->capacity*type_size);
	DeAllocate(arr->allocator,(arr->start));
	arr->capacity = new_capacity;
	arr->start = new_mem;
}

void _maybeGrow(DynamicArray_void *arr, size_t type_size)
{
	if (arr->length >= arr->capacity)
		_grow_to(arr, type_size,arr->capacity * DA_GROWTHRATE);
}

void _insert_space(DynamicArray_void *arr, int index, size_t type_size)			
{																				
	assert(0 <= index);
	_maybeGrow((DynamicArray_void *)arr, type_size);
	arr->length++;
	if (index >= arr->length)
	{
		assert(index == arr->length);
		return;
	}
	void *source = ((char *)arr->start)+(type_size*index); //&arr[index]											
	void *dest = ((char *)source) + type_size;				//&arr[index+1]
	size_t bytes = (arr->length - 1 - index)*type_size;	/*-1 since it's allready been incremented*/		
	memmove(dest, source, bytes);													
}

#define DHDS_constructDA(type, initialCapacity,allocator) constructDynamicArray_##type(initialCapacity,DHMA_LOCATION,allocator)
#define DHDS_ORD_constructDA(type, initialCapacity,allocator) ORD_constructDynamicArray_##type(initialCapacity, DHMA_LOCATION,allocator)

// typesafe dynamic arrays wrappers
#define DEFINE_DynamicArray(type)		\
struct DynamicArray_##type				\
{										\
	DynamicArray_StructMeat(type)		\
};										\
										\
DynamicArray_##type DynamicArrayFromFixedSize_##type(type *tree, int length, const char*allocationInfo, DH_Allocator allocator=default_allocator)	\
{																				\
	DynamicArray_##type darr = {};												\
	darr.allocator = allocator;													\
	darr.allocationInfo = (void *)allocationInfo;								\
	darr.start = (type *)Allocate(allocator,(length * DA_GROWTHRATE)* sizeof(type),(void *)allocationInfo);\
	darr.length = length;														\
	darr.capacity = length * DA_GROWTHRATE;										\
	memcpy(darr.start,tree,length*sizeof(type));								\
	return darr;																\
}																				\
																				\
DynamicArray_##type constructDynamicArray_##type(int initialCapacity,const char*allocationInfo, DH_Allocator allocator= default_allocator)	\
{																				\
	DynamicArray_##type darr = {};												\
	darr.allocationInfo = (void *)allocationInfo;								\
	darr.allocator = allocator;													\
	darr.start = (type *)Allocate(allocator, initialCapacity* sizeof(type), (void *)allocationInfo);	\
	darr.length = 0;															\
	darr.capacity = initialCapacity;											\
	return darr;																\
}																				\
																				\
void Add(DynamicArray_##type *tree, type elem)									\
{																				\
	_maybeGrow((DynamicArray_void *)tree,sizeof(type));							\
	tree->start[tree->length++] = elem;											\
}																				\
																				\
void Remove(DynamicArray_##type *tree, int index)								\
{																				\
	assert(tree->length > index);												\
	tree->start[index] = tree->start[--tree->length];								\
}																				\
void Insert(DynamicArray_##type *tree, type element,int index)					\
{																				\
	_insert_space((DynamicArray_void *)tree, index, sizeof(type));				\
	tree->start[index]=element;													\
}																				\
																				\
void RemoveOrd(DynamicArray_##type *tree, int index)								\
{																				\
	void *source = &tree->start[index+1];										\
	void *dest = &tree->start[index];											\
	size_t bytes = (--tree->length-index)*sizeof(type);							\
	memmove(dest,source,bytes);													\
}\



bool binary_search(void *arr, size_t length, size_t type_size, void *elem, int(*comparator)(void *a, void *b), int *_out_index = 0)
{
	//returns the index where the new element would be inserted if not found, ie 1 3 6, bs(4) -> 2
	if (length == 0)
	{
		*_out_index = 0;
		return false;
	}

	char *_arr = (char *)arr;
	int low = -1;
	int high = length;
	for (;;)
	{
		int center = (low + high) / 2;
		int cmp = comparator((_arr + center * type_size),elem);
		if (low == center)
		{
			if(_out_index)
				*_out_index = cmp ? high : low;
			return !cmp;
		}
		if (high == center||!cmp) // snipe or smaller than first elem.
		{
			if (_out_index)
				*_out_index = center;
			return !cmp;
		}
		if (cmp > 0)
		{
			high = center;
		}
		else
		{
			low = center;
		}
	}
}

bool linear_search(void *arr, size_t length, size_t type_size, void *elem, int(*comparator)(void *a, void *b), int *_out_index = 0)
{
	//maybe use simd or someother cool shit if the elemets are small enough.
	char *_arr = (char *)arr;
	for (int i = 0; i < length; i++)
	{
		int cmp = comparator(_arr + i * type_size, elem);
		if (cmp>=0)
		{
			if (_out_index)
				*_out_index = i;
			return !cmp;
		}
	}
	if (_out_index)
		*_out_index = length;
	return false;
}

bool search(void *arr, size_t length, size_t type_size, void *elem, bool (*eq)(void *a, void *b), int *_out_index = 0)
{
	//maybe use simd or someother cool shit if the elemets are small enough.
	char *_arr = (char *)arr;
	for (int i = 0; i < length; i++)
	{
		if (eq(_arr + i * type_size, elem))
		{
			if (_out_index)
				*_out_index = i;
			return true;
		}
	}
	if (_out_index)
		*_out_index = length;
	return false;
}




// a wrapper for the dynamic array specifying a particular order.
// assumes that a dynamic array of the same type is allready specified
// you may cast it to a normal dynamic array if you wish
// mutliple orders on the same type (say integer in acending and lexografical order) 
// is best done with typdefs.
// ie. 
//		typdef int lex_int
//		DEFINE_DynamicArray(lex_int)
//		DEFINE_ORD_DynamicArray(lex_int, &lex_compare);
//  
//		typdef int asc_int
//		DEFINE_DynamicArray(asc_int)
//		DEFINE_ORD_DynamicArray(asc_int, &asc_compare);

enum  ordered_insert_placement
{
	ordered_insert_dont_care,
	ordered_insert_first,
	ordered_insert_last,
};

#define DEFINE_ORD_DynamicArray(type, comparator)\
struct ORD_DynamicArray_##type\
{\
	DynamicArray_StructMeat(type)		\
};\
\
bool binary_search(ORD_DynamicArray_##type *tree, type elem, int *_out_index)\
{\
	return binary_search((void *)tree->start,(size_t)tree->length,sizeof(type),&elem,comparator,_out_index);\
}\
bool linear_search(ORD_DynamicArray_##type *tree, type elem, int *_out_index)\
{\
	return linear_search((void *)tree->start,(size_t)tree->length,sizeof(type),&elem,comparator,_out_index);\
}\
void Insert(ORD_DynamicArray_##type *tree, type elem, ordered_insert_placement placement)\
{\
	int index;\
	binary_search(tree, elem, &index);\
	if(placement == ordered_insert_first)\
	{\
		while (index > 0)\
		{\
			if (comparator(&tree->start[index], &elem))\
				break;\
			--index;\
		}\
	}\
	if (placement == ordered_insert_last)\
	{\
		while (index < tree->length)\
		{\
			if (comparator(&tree->start[index], &elem))\
				break;\
			++index;\
		}\
	}\
	Insert((DynamicArray_##type *)tree, elem, index);\
}\
void Remove(ORD_DynamicArray_##type *tree, int index)\
{\
	RemoveOrd((DynamicArray_##type *)tree, index);\
}\
\
ORD_DynamicArray_##type ORD_DynamicArrayFromFixedSize(type *tree, int length, const char*allocationInfo)	\
{																				\
	return *(ORD_DynamicArray_##type *) &DynamicArrayFromFixedSize_##type(tree, length, allocationInfo);\
}																				\
\
ORD_DynamicArray_##type ORD_constructDynamicArray_##type(int initialCapacity, const char*allocationInfo,DH_Allocator allocator = default_allocator)	\
{\
	return *(ORD_DynamicArray_##type *) &constructDynamicArray_##type(initialCapacity, allocationInfo); \
}
#define DEFINE_Complete_ORD_DynamicArray(type, comparator)\
DEFINE_DynamicArray(type)\
DEFINE_ORD_DynamicArray(type,comparator)

#define HT_OPTIMAL_LOAD_FACTOR .5
#define HT_GROWTHRATE 2

#define DHDS_constructHT(key_type,value_type, initialCapacity,allocator)construct_HashTable_##key_type##_##value_type(initialCapacity, "HashTable:" DHMA_STRING_(DHMA_LOCATION_), allocator);



// a standard hashtable implementation. if the same key is inserted multiple times the value of the last insert will remain.
// at some point we might support specifying other behviours.

enum hashtable_bucket_state
{
	bucket_state_empty,
	bucket_state_deleted,
	bucket_state_occupied,
};

unsigned int silly_hash(int i){
	return (unsigned int)i * (unsigned int)2654435761;
}

unsigned long long silly_hash_lli(long long int i) {
	return (unsigned long long)i * (unsigned long long int)2654435761;
}


void compatible_free(void *mem_to_free, void *allocation_info)
{
	free_(mem_to_free);
}
static DH_Allocator default_allocator = { &alloc_, &compatible_free };


#define DEFINE_HashTable(key_type, value_type, hash_function,key_equality_func)\
struct HashBucket_##key_type##_##value_type\
{\
	hashtable_bucket_state state;\
	key_type key;\
	value_type value;\
};\
\
struct HashTable_##key_type##_##value_type\
{\
	HashBucket_##key_type##_##value_type *buckets;\
	int length;\
	int capacity;\
	DH_Allocator allocator;\
	void *allocationInfo;\
};\
\
int HT_count(HashTable_##key_type##_##value_type *hashtable)\
{\
	int c = 0;\
	for (int i = 0; i < hashtable->capacity; i++)\
	{\
		if (hashtable->buckets[i].state == bucket_state_occupied) ++c;\
	}\
	return c;\
}\
\
HashTable_##key_type##_##value_type construct_HashTable_##key_type##_##value_type(int capacity, void *allocationInfo, DH_Allocator allocator)\
{\
	HashTable_##key_type##_##value_type new_hashtable = {};\
	new_hashtable.allocator = allocator;\
	new_hashtable.buckets = (HashBucket_##key_type##_##value_type *)Allocate(new_hashtable.allocator,(capacity * sizeof(HashBucket_##key_type##_##value_type)), allocationInfo);\
	for(int i = 0; i<capacity;i++) new_hashtable.buckets[i].state = bucket_state_empty;\
	new_hashtable.capacity = capacity;\
	new_hashtable.allocationInfo = allocationInfo;\
	assert(HT_count(&new_hashtable) == new_hashtable.length && "create");\
	return new_hashtable;\
}\
\
void _maybe_grow(HashTable_##key_type##_##value_type *hashtable);\
\
value_type *insert(HashTable_##key_type##_##value_type *hashtable, key_type key, value_type value)\
{\
	_maybe_grow(hashtable);\
	unsigned int index = ((unsigned int)hash_function(key)) % hashtable->capacity;\
	int inital_index = index;\
	int iterations =0;\
	for(int i = 0; i< hashtable->capacity; i++)\
	{\
		assert(index >=0 && index < hashtable->capacity);\
		++iterations;\
		if(hashtable->buckets[index].state != bucket_state_occupied|| key_equality_func(hashtable->buckets[index].key, key)) break; \
		++index;\
		index %= hashtable->capacity;\
	}\
	assert(index >=0 && index < hashtable->capacity);\
	HashBucket_##key_type##_##value_type bucket = {};\
	bucket.state = bucket_state_occupied;\
	bucket.key = key;\
	bucket.value = value;\
	hashtable->buckets[index] = bucket;\
	++hashtable->length;\
	return &hashtable->buckets[index].value;\
}\
\
void _maybe_grow(HashTable_##key_type##_##value_type *hashtable)\
{\
	float new_load_factor =((float)hashtable->length + 1) / ((float)hashtable->capacity);\
	if ( new_load_factor > HT_OPTIMAL_LOAD_FACTOR)\
	{\
		int prev_len = hashtable->length;\
		HashTable_##key_type##_##value_type new_hashtable = construct_HashTable_##key_type##_##value_type(hashtable->capacity*HT_GROWTHRATE, hashtable->allocationInfo,hashtable->allocator);\
		for (int i = 0; i < hashtable->capacity; i++)\
		{\
			assert(i >=0 && i < hashtable->capacity);\
			if (hashtable->buckets[i].state == bucket_state_occupied)\
			{\
				insert(&new_hashtable, hashtable->buckets[i].key, hashtable->buckets[i].value);\
			}\
		}\
		hashtable->allocator.free(hashtable->buckets,hashtable->allocationInfo);\
		assert(prev_len == new_hashtable.length);\
		*hashtable = new_hashtable;\
	}\
}\
\
HashBucket_##key_type##_##value_type *lookup_bucket(HashTable_##key_type##_##value_type *hashtable, key_type key, bool *success)\
{\
	int index = ((unsigned int) hash_function(key)) % hashtable->capacity;\
	for(int i = 0; i< hashtable->capacity; i++)\
	{\
		assert(index >=0 && index < hashtable->capacity);\
		if(hashtable->buckets[index].state == bucket_state_empty) break; \
		HashBucket_##key_type##_##value_type bucket = hashtable->buckets[index];\
		if (bucket.state == bucket_state_occupied)\
		{\
			if (key_equality_func(bucket.key, key))\
			{\
				if(success)\
					*success = true;\
				return &hashtable->buckets[index];\
			}\
		}\
		++index;\
		index %= hashtable->capacity;\
	}\
	if(success)\
		*success = false;\
	return (HashBucket_##key_type##_##value_type *)0;\
}\
bool lookup(HashTable_##key_type##_##value_type *hashtable, key_type key, value_type ** _out_res)\
{\
	bool success;\
	HashBucket_##key_type##_##value_type *bucket = lookup_bucket(hashtable, key, &success); \
	if(_out_res){\
		*_out_res = success ? &bucket->value : (value_type *)0;\
	}\
	return success;\
}\
\
void remove(HashTable_##key_type##_##value_type *hashtable, key_type key, bool *success)\
{\
	bool succ;\
	HashBucket_##key_type##_##value_type *bucket = lookup_bucket(hashtable, key, &succ);\
	if(success) *success = succ;\
	if (succ)\
	{\
		bucket->state = bucket_state_deleted;\
		--hashtable->length;\
	}\
}\
void clear(HashTable_##key_type##_##value_type *hashtable)\
{\
	HashBucket_##key_type##_##value_type empty_bucket;\
	empty_bucket.state = bucket_state_empty;\
\
	for (int i = 0; i < hashtable->capacity; i++)\
	{\
		hashtable->buckets[i] = empty_bucket;\
	}\
	hashtable->length = 0;\
}
// if we have a rather big array and want to have some data on a small amount of those elements
// we'd rather not put data on every element and the only use it on some of those elements
// its better practice to keep a seperate array, saving that data and keeping track of which element that data belongs to
// this implementation also works with multiple data elements on a single array element. 
// if you have multiple data element on a single array element you'll get them in the order they appear in the sparse array, insert carefully.

#define intrusive_sparse_array_body int sparseArray_target_index
#define DEFINE_SparseCompare(type)\
int sparse_array_cmp_##type(void *a, void *b)\
{\
	return ((type*)a)->sparseArray_target_index - ((type *)b)->sparseArray_target_index;\
}\

#define DEFINE_SparseArray(type)\
struct SparseArrayIterator_##type\
{\
	int array_index;\
};\
SparseArrayIterator_##type  make_iterator(ORD_DynamicArray_##type *tree, int index)\
{\
	SparseArrayIterator_##type ret = {};\
	type elem = {};\
	elem.sparseArray_target_index = index;\
	int found_index;\
	bool found = binary_search(tree->start, tree->length, sizeof(type), &elem, sparse_array_cmp_##type, &found_index);\
	if(found)\
	{\
		while (found_index > 0)\
		{\
			if (tree->start[found_index].sparseArray_target_index != tree->start[found_index-1].sparseArray_target_index)\
			{\
				break;\
			}\
			else --found_index;\
		}\
	}\
	ret.array_index = found_index-1;\
	return ret;\
}\
SparseArrayIterator_##type make_iterator(ORD_DynamicArray_##type *tree)\
{\
	return make_iterator(tree,0);\
}\
bool get_next(ORD_DynamicArray_##type *tree, SparseArrayIterator_##type *it,int target_index, type *_out_element)\
{\
	if(tree->length == 0)return false;\
	if(tree->start[it->array_index+1].sparseArray_target_index  == target_index)\
	{\
		*_out_element = tree->start[++it->array_index];\
		return true;\
	}\
	return false;\
}

struct test
{
	int payload;
	intrusive_sparse_array_body;
};

DEFINE_DynamicArray(int);
typedef int deffed_int;
DEFINE_DynamicArray(deffed_int);

/*
DEFINE_DynamicArray(test);
DEFINE_SparseComapre(test);
DEFINE_ORD_DynamicArray(test, sparse_array_cmp_test);
DEFINE_SparseArray(test); //...yes you need to define all these..... sorry
*/

#define DEFINE_Complete_SparseArray(type)\
DEFINE_SparseCompare(type);\
DEFINE_Complete_ORD_DynamicArray(type, sparse_array_cmp_##type);\
DEFINE_SparseArray(type);\

DEFINE_Complete_SparseArray(test);

/*
void test_func()
{
	DynamicArray_int base_arr = DHDS_constructDA(int, 20);
	ORD_DynamicArray_test sparse_arr = ORD_constructDynamicArray_test(20, "");
	
	Add(&base_arr, 1);
	Add(&base_arr, 2);
	Add(&base_arr, 3);
	Add(&base_arr, 4);
	Add(&base_arr, 5);
	Add(&base_arr, 6);
	Add(&base_arr, 7);
	Add(&base_arr, 8);
	Insert(&base_arr, 9, 0);
	Insert(&base_arr, 11, 9);

	test t = {};
	t.payload = 100;
	t.sparseArray_target_index = 5;
	ordered_insert_placement place = ordered_insert_last;
	Insert(&sparse_arr, t, place);
	t.payload = 10;
	t.sparseArray_target_index = 2;
	Insert(&sparse_arr, t, place);
	t.payload = 20;
	t.sparseArray_target_index = -1;
	Insert(&sparse_arr, t, place);
	t.payload = 200;
	t.sparseArray_target_index = 100;
	Insert(&sparse_arr, t, place);

	SparseArrayIterator_test it = make_iterator(&sparse_arr,0);
	for (int i = 0; i < base_arr.length;i++)
	{
		test elem;
		while (get_next(&sparse_arr,&it,i,&elem))
		{
			volatile int q = elem.payload;
		}
	}
}
*/
#define RETURN_SUCCESS_AND_VALUE(cond,_out_prop,_out_value)\
if (cond)\
{\
	if(_out_prop)\
	*_out_prop = _out_value;\
	return true;\
}\
return false;

// concept might be generalized if the concept proves to need it.
// for now the concrete version works.

bool binsumtree_index_from_leef_index(DynamicArray_int *arr, int current_index, int *_out_res_index)
{
	RETURN_SUCCESS_AND_VALUE(current_index < arr->capacity / 2, _out_res_index, current_index + arr->capacity / 2);
}

bool binsumtree_leef_index_from_index(DynamicArray_int *arr, int current_index, int *_out_res_index)
{
	RETURN_SUCCESS_AND_VALUE(current_index >= arr->capacity / 2, _out_res_index, current_index - arr->capacity / 2);
}
bool binsumtree_left(DynamicArray_int *arr, int current_index, int *_out_res_index)
{
	RETURN_SUCCESS_AND_VALUE(current_index < arr->capacity / 2, _out_res_index, current_index * 2);
}

bool binsumtree_right(DynamicArray_int *arr, int current_index, int *_out_res_index)
{
	RETURN_SUCCESS_AND_VALUE(current_index < arr->capacity / 2, _out_res_index, current_index * 2+1);
}

bool binsumtree_parent(DynamicArray_int *arr, int current_index, int *_out_res_index)
{
	if (current_index != 1)
	{
		*_out_res_index = current_index / 2;
		return true;
	}
	return false;
}
int binsumtree_recalc_sum(DynamicArray_int *tree, int index=1)
{
	int l, r;
	if (!binsumtree_left(tree, index, &l) || !binsumtree_right(tree, index, &r)) return tree->start[index];
	return tree->start[index] = binsumtree_recalc_sum(tree, l) + binsumtree_recalc_sum(tree, r);
}
int binsumtree_get_length(DynamicArray_int *tree)
{
	int len = 0;
	for (int i = tree->capacity / 2; i <tree->capacity; i++)
		if (tree->start[i])++len;
	return len;
}



void binsumtree_double_size(DynamicArray_int *tree)
{
	size_t new_capacity = tree->capacity * 2;
	int *new_mem = (int *)Allocate(tree->allocator,new_capacity * sizeof(int), tree->allocationInfo);
	memset(new_mem, 0, new_capacity * sizeof(int));
	memmove(&new_mem[new_capacity / 2], &tree->start[tree->capacity / 2], tree->capacity*sizeof(int) / 2);
	DeAllocate(tree->allocator, tree->start);
	tree->capacity = new_capacity;
	tree->start = new_mem;
	binsumtree_recalc_sum(tree);
}

void binsumtree_set_relative(DynamicArray_int *tree, int index, int delta)
{
	while (index>=tree->capacity / 2)binsumtree_double_size(tree);
	if (!binsumtree_index_from_leef_index(tree, index, &index))return;
	tree->start[index] += delta;
	while (binsumtree_parent(tree, index, &index))
	{
		tree->start[index] += delta;
	}
	binsumtree_recalc_sum(tree);
}

void binsumtree_set(DynamicArray_int *tree, int index, int elem)
{
	while (index>=tree->capacity / 2)binsumtree_double_size(tree);
	if (!binsumtree_index_from_leef_index(tree, index, &index))
	{
		assert(false);
		return;
	}
	int delta = elem- tree->start[index];
	tree->start[index] = elem;
	while (binsumtree_parent(tree, index, &index))
	{
		tree->start[index] += delta;
	}
	binsumtree_recalc_sum(tree);
}

int binsumtree_left_most_leef(DynamicArray_int *tree, int index)
{
	for (;;) 
		if (!binsumtree_left(tree, index, &index))break;
	return index;
}
bool binsumtree_leef_value(DynamicArray_int *tree, int leef_index, int *value)
{
	int index;
	bool success = binsumtree_index_from_leef_index(tree, leef_index, &index);
	RETURN_SUCCESS_AND_VALUE(success, value, tree->start[index]);
}
bool binsumtree_search(DynamicArray_int *tree, int elem, int* _out_result_index, int index = 1)
{
	if (elem <= 0)
	{
		int left_leef_index = binsumtree_left_most_leef(tree, index);
		binsumtree_leef_index_from_index(tree, left_leef_index, _out_result_index);
		return true;
	}
	if (tree->start[index] <= elem) return false;
	int left_index = 0;
	if (!binsumtree_left(tree, index, &left_index))
	{
		binsumtree_leef_index_from_index(tree, index, _out_result_index);
		return true;
	}
	if (binsumtree_search(tree, elem, _out_result_index, left_index))
		return true;

	int right_index = 0;
	if (!binsumtree_right(tree, index, &right_index))
	{
		binsumtree_leef_index_from_index(tree, index, _out_result_index);
		*_out_result_index = index;
		return true;
	}
	if (binsumtree_search(tree, elem-tree->start[left_index], _out_result_index, right_index))
		return true;
	return false;
}


int binsumtree_getSumUntil(DynamicArray_int *tree, int index)
{
	if (!binsumtree_index_from_leef_index(tree, index, &index))return 0;
	
	int ack = 0;
	while(index != 1) {
		if (index % 2 == 1)
		{
			ack += tree->start[index-1];
		}
		binsumtree_parent(tree, index, &index);
	}
	return ack;
}


void binsumtree_maybegrow(DynamicArray_int *tree)
{
	if (tree->start[tree->capacity - 2] != 0)binsumtree_double_size(tree);
}

void binsumtree_insert(DynamicArray_int *tree, int leef_index, int elem)
{
	binsumtree_maybegrow(tree);
	int index;
	if (!binsumtree_index_from_leef_index(tree, leef_index, &index))return;
	memmove(&tree->start[index + 1], &tree->start[index], (tree->capacity - (index+1))*sizeof(int));
	tree->start[index] = elem;
	binsumtree_recalc_sum(tree);
}

void binsumtree_remove(DynamicArray_int *tree, int index)
{
	if (!binsumtree_index_from_leef_index(tree, index, &index))return;
	memmove(&tree->start[index], &tree->start[index+1], (tree->capacity - (index+1))*sizeof(int));
	tree->start[tree->capacity - 1] = 0;
	binsumtree_recalc_sum(tree);
}
/*
void test_binsumtree_()
{
	DynamicArray_int tree = DHDS_constructDA(int, 4);
	memset(tree.start, 0, tree.capacity*sizeof(int));
	binsumtree_insert(&tree, 0, 2);
	binsumtree_insert(&tree, 1, 5);
	assert(binsumtree_getSumUntil(&tree, 0)==0);
	assert(binsumtree_getSumUntil(&tree, 1)==2);
	binsumtree_insert(&tree, 1, 2);
	binsumtree_insert(&tree, 1, 3); //@this is doing something wierd... (ie is fucking wrong)
	assert(binsumtree_getSumUntil(&tree, 0) == 0);
	assert(binsumtree_getSumUntil(&tree, 1) == 2);
	assert(binsumtree_getSumUntil(&tree, 2) == 5);
	assert(binsumtree_getSumUntil(&tree, 3) == 7);
	//2325
	binsumtree_remove(&tree, 1);
	binsumtree_remove(&tree, 2);
	//22
	assert(binsumtree_getSumUntil(&tree, 3) == 4);
	assert(binsumtree_getSumUntil(&tree, 1) == 2);

	DynamicArray_int tree2 = DHDS_constructDA(int, 200);
	for (int i = 1; i < 200; i++)
	{
		int res;
		if (binsumtree_parent(&tree2, i, &res))
		{
			int lr,rr;
			binsumtree_left(&tree2, res, &lr);
			binsumtree_right(&tree2, res, &rr);
			assert(lr == i || rr == i && rr==lr+1);
		}

		if (binsumtree_left(&tree2,i,&res))
		{
			int res2;
			binsumtree_parent(&tree2, res, &res2);
			assert(i == res2);
		}

		if (binsumtree_right(&tree2, i, &res))
		{
			int res2;
			binsumtree_parent(&tree2, res, &res2);
			assert(i == res2);
		}
	}
}
*/

#endif









