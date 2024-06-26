#ifndef potato_memory_h
#define potato_memory_h

#include "common.h"
#include "object.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, ptr, oldCount, newCount) \
    (type*)reallocate((ptr), sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, ptr, oldCount) \
    reallocate((ptr), sizeof(type) * (oldCount), 0)

void* reallocate(void* ptr, size_t oldSize, size_t newSize);

#define ALLOCATE(type, size) (type*)reallocate(NULL, 0, sizeof(type) * (size))

#define FREE(type, ptr) \
    reallocate(ptr, sizeof(type), 0)

void freeObjects();

#endif
