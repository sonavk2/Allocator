#include <stddef.h>

/** Called once before any other function here; argument is the smallest usable address */
void allocator_init(void *newbase);

/** Called once before each test case; should free any used memory and reset for the next test */
void allocator_reset();

/** Like malloc but using the memory provided to allocator_init */
void *mymalloc(size_t size);
/** Like free but using the memory provided to allocator_init */
void myfree(void *ptr);
/** Like realloc but using the memory provided to allocator_init */
void *myrealloc(void *ptr, size_t size);
