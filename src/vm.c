#include "vm.h"
#include "debug.h"
#include "compiler.h"

VM vm;

static void resetStack() {
    vm.sp = vm.stack;
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

void initVM() {
    resetStack();
}

void exitVM() {
    return;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[(READ_BYTE()<<24) + (READ_BYTE()<<16) + (READ_BYTE()<<8) + (READ_BYTE())])
#define BINARY_OP(op) do {vm.sp[-2] = vm.sp[-2] op vm.sp[-1]; vm.sp--;} while(0);
    
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

        uint8_t instruction;
        switch(instruction = READ_BYTE()) {
            case OP_ADD:
                BINARY_OP(+);
                break;
            case OP_SUBTRACT:
                BINARY_OP(-);
                break;
            case OP_MULTIPLY:
                BINARY_OP(*);
                break;
            case OP_DIVIDE:
                BINARY_OP(/);
                break;
            case OP_NEGATE:
                vm.sp[-1] = -vm.sp[-1];
                break;
            case OP_CONSTANT_LONG:
                push(READ_LONG_CONSTANT());
                break;
            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;
            case OP_RETURN:
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
        }
    }

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