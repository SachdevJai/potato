#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

void* reallocate(void* ptr, size_t oldSize, size_t newSize) {
    if(newSize == 0) {
        free(ptr);
        return NULL;
    }

    void* result = realloc(ptr, newSize);
    if(result == NULL) exit(1);
    return result;
}

static void freeObject(Obj* object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
    }
}

void freeObjects() {
    Obj* objects = vm.objects;
    while(objects != NULL) {
        Obj* next = objects->next;
        freeObject(objects);
        objects = next;
    }
}