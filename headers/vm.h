#ifndef potato_vm_h
#define potato_vm_h

#include <stdarg.h>

#include "common.h"
#include "value.h"
#include "object.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* sp;
    Table strings;
    Obj* objects;
} VM;

extern VM vm;

void initVM();
void exitVM();

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif