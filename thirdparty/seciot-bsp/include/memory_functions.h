#ifndef MEMORY_FUNCTIONS_H
#define MEMORY_FUNCTIONS_H
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>

#include "defines.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

void Heap_Init();

void* memcpy(void* dest, const void* src, size_t n);

void* memset(void* ptr, int value, size_t n);

int memcmp(const void* ptr1, const void* ptr2, size_t n);

/* Allocate SIZE bytes of memory.  */
/** @fn  void *malloc(size_t size)
 * @brief  this function is used to dynamically allocate memory
 * @details  when this function is called memory is allocated in the heap during runtime
 * @param size_t size - size of the block to be allocated
 * @return  returns a pointer to the assigned block
 *          returns NULL if allocation fails
 */
extern void *malloc (size_t size);

/* Free a block allocated by `malloc', `realloc' or `calloc'.  */
/** @fn  void free(void *ptr)
 * @brief frees the memory assigned by malloc
 * @details  frees the memory assigned by malloc and when freed this memory is added back to the list to be reused
 * @param void *ptr - pointer to the memory assigned by malloc
 */
extern void free (void *ptr);
extern void *calloc(size_t size1,size_t size2);
#ifdef __cplusplus
}
#endif
#endif