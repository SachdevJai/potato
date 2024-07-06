#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "vm.h"
#include "memory.h"

VM vm;

static void resetStack() {
    vm.sp = vm.stack;
    vm.objects = NULL;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t intsruction = vm.ip - vm.chunk->code - 1;
    int line = getLine(vm.chunk, intsruction);
    fprintf(stderr, "[Line %d] in script", line);
    resetStack();
}

void push(Value value) {
    if(vm.sp < vm.stack + STACK_MAX) {
        *vm.sp = value;
        vm.sp++;
    } else {
        printf("STACKOVERFLOW\n");
        exit(1);
    }
}

Value pop() {
    if(vm.sp > vm.stack) {
        vm.sp--;
        return *vm.sp;
    } else {
        printf("Cannot pop from stack: Stack empty\n");
        exit(1);
    }
}

static Value peek(int distance) {
  return vm.sp[-1 - distance];
}

void initVM() {
    resetStack();
    initTable(&vm.strings);
    initTable(&vm.globals);
}

void exitVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    freeObjects();
}

static Value concatenate(Value string1, Value string2) {
    ObjString* a = AS_STRING(string1);
    ObjString* b = AS_STRING(string2);

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    return OBJ_VAL(result);
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[(READ_BYTE()<<24) + (READ_BYTE()<<16) + (READ_BYTE()<<8) + (READ_BYTE())])
#define BINARY_OP(valueType, op) do { \
    if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers"); \
        return INTERPRET_RUNTIME_ERROR; \
    } \
    vm.sp[-2] = NUMBER_VAL(AS_NUMBER(vm.sp[-2]) op AS_NUMBER(vm.sp[-1])); \
    vm.sp--; \
} while(0);
#define READ_STRING() AS_STRING(READ_CONSTANT());
    
    for(;;) {

        #ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.sp; slot++) {
        printf("[ ");
        printValue(*slot);
        printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk,(int)(vm.ip - vm.chunk->code));
        #endif

        uint8_t instruction = READ_BYTE();
        switch(instruction) {
            case OP_ADD:
                if(IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    Value b = pop();
                    Value a = pop();
                    push(concatenate(a, b));
                    break;
                } else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    BINARY_OP(NUMBER_VAL, +);
                    break;
                } else {
                    runtimeError("Operands must be two numbers or two strings");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;

            case OP_SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;

            case OP_MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;

            case OP_DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;

            case OP_NEGATE:
                if(!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm.sp[-1] = NUMBER_VAL(-AS_NUMBER(vm.sp[-1]));
                break;

            case OP_CONSTANT_LONG:
                push(READ_LONG_CONSTANT());
                break;

            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;

            case OP_RETURN:
                // exit compiler
                return INTERPRET_OK;

            case OP_NIL: 
                push(NIL_VAL);
                break;

            case OP_TRUE:
                push(BOOL_VAL(1));
                break;

            case OP_FALSE:
                push(BOOL_VAL(0));
                break;

            case OP_NOT:
                vm.sp[-1] = NUMBER_VAL(
                    vm.sp[-1].type == IS_NIL(vm.sp[-1]) || 
                    IS_BOOL(vm.sp[-1]) && !AS_BOOL(vm.sp[-1]) || 
                    IS_NUMBER(vm.sp[-1]) && !AS_NUMBER(vm.sp[-1])
                );
                break;

            case OP_EQUAL:
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;

            case OP_GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;

            case OP_LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;

            case OP_POP:
                pop();
                break;

            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                break;
            }

            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if(!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }

            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if(tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }

            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }

            
        }
    }

#undef READ_STRING
#undef BINARY_OP
#undef READ_LONG_CONSTANT
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if(!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}