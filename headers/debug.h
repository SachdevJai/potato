#ifndef potato_debug_h
#define potato_debug_h

#include "common.h"
#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);
int getLine(Chunk* chunk, int offset);

#endif
