#ifndef potato_chunk_h
#define potato_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_RETURN,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_PRINT,
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
} OpCode;

typedef struct {
    int first;
    int second;
} intPair;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
    int lineCapacity;
    int lineCount;
    intPair* lines; // [[1,2],[2,1],[3,4]] -> 1, 1, 2, 3, 3, 3, 3 
} Chunk;


void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
void freeChunk(Chunk* chunk);

int addConstant(Chunk* chunk, Value value);
void writeConstant(Chunk* chunk, Value value, int line);

#endif