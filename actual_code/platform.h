#ifndef DH_PLATFORM
#define DH_PLATFORM
// these function will be called for big allocations
// definitely not less than 4kb, probably not more than 64
void *platform_alloc(size_t bytes_to_alloc); 
void platform_free(void *mem);
BackingBuffer *platform_execute_command(char *command, int command_len);


#endif
