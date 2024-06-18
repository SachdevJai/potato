#ifndef potato_object_h
#define potato_object_h

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "value.h"

typedef enum {
    OBJ_STRING,
} ObjectType;

struct Obj{
    ObjectType type;
    Obj* next;
};

struct ObjString{
    Obj obj;
    int length;
    char* chars;
};

ObjString* copyString(const char* chars, int length);

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

void printObject(Value value);
ObjString* takeString(char* chars, int length);

static inline bool isObjType(Value value, ObjectType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif