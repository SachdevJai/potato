#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void initValueArray(ValueArray* array) {
	array->values = NULL;
	array->capacity = 0;
	array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
	if (array->capacity < array->count + 1) {
		int oldCapacity = array->capacity;
		array->capacity = GROW_CAPACITY(oldCapacity);
		array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
	}
	array->values[array->count] = value;
	array->count++;
}

void freeValueArray(ValueArray* array) {
	FREE_ARRAY(Value, array->values, array->capacity);
}

void printValue(Value value) {
	switch (value.type) {
	case VAL_BOOL:
		printf(AS_BOOL(value) ? "true": "false");
		break;
	case VAL_NIL:
		printf("nil");
		break;
	case VAL_NUMBER:
		printf("%g", AS_NUMBER(value));
		break;
	case VAL_OBJ:
		printObject(value);
		break;
	}
}

static Value typecastToNumber(Value value) {
	switch (value.type) {
		case VAL_NUMBER: return value;
		case VAL_BOOL: return NUMBER_VAL((double)value.as.boolean);
		case VAL_NIL: return NUMBER_VAL((double)value.as.number);
		default: return NIL_VAL;
	}
}

bool valuesEqual(Value value1, Value value2) {
	switch (value1.type) {
		case VAL_BOOL:   return AS_BOOL(value1) == AS_BOOL(value2);
		case VAL_NIL:    return true;
		case VAL_NUMBER: return AS_NUMBER(value1) == AS_NUMBER(value2);
		case VAL_OBJ: {
			ObjString* aString = AS_STRING(value1);
			ObjString* bString = AS_STRING(value2);
			return aString->length == bString->length &&
				memcmp(aString->chars, bString->chars,
						aString->length) == 0;
		}
		default:         return false;
	}
}