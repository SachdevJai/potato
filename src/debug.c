#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "object.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("==========%s==========\n", name);

    for(int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char* OpCode, Chunk* chunk, int offset) {
    uint8_t constant = chunk->code[offset+1];
    printf("%-16s %4d '", OpCode, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset+2;
}

static int longConstantInstruction(const char* OpCode, Chunk* chunk, int offset) {
    uint32_t constant = (uint32_t)(chunk->code[offset + 1]) |
                        ((uint32_t)(chunk->code[offset + 2]) << 8) |
                        ((uint32_t)(chunk->code[offset + 3]) << 16);

    printf("%-16s %4d '", OpCode, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

static int simpleInstruction(const char* OpCode, int offset) {
    printf("%s\n", OpCode);
    return offset + 1;
}

int getLine(Chunk* chunk, int offset) {
    
    int index = 0;
    for(int i = 0; i < chunk->lineCount; i++) {
        index += chunk->lines[i].second;
        if(index > offset) {
            return chunk->lines[i].first;
        }
    }

    printf("INVALID OFFSET, LINE OUT OF BOUNDS");
    exit(1);
}

int disassembleInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    printf("%04d ", getLine(chunk, offset));

    uint8_t instruction = chunk->code[offset];

    switch (instruction) {
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUALS", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESSER", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
