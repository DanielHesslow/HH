// these function will be called for big allocations
// definitely not less than 4kb, probably not more than 64
void *platform_alloc(size_t bytes_to_alloc); 
void platform_free(void *mem);


