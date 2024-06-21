#ifndef potato_table_h
#define potato_table_h

#include "value.h"
#include "common.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    Entry* entries;
    int count;
    int capacity;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, ObjString* key, Value value);
void tableAddAll(Table* from, Table* to);
bool tableGet(Table* table, ObjString* key, Value* value);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

#endif