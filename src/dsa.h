#ifndef DSA_H
#define DSA_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h> 

#define DECLARE_DA(type, name) \
typedef struct { \
    type *data; \
    size_t size; \
    size_t capacity; \
} name##_DA; \
\
name##_DA* init_##name##_dynamic_array(size_t initial_capacity) { \
    name##_DA *array = (name##_DA *)malloc(sizeof(name##_DA)); \
    array->data = (type *)malloc(initial_capacity * sizeof(type)); \
    array->size = 0; \
    array->capacity = initial_capacity; \
    return array; \
} \
\
void append_##name##_DA(name##_DA *array, type value) { \
    if(array->size >= array->capacity) { \
        array->capacity *= 2; \
        array->data = (type *)realloc(array->data, array->capacity * sizeof(type)); \
    } \
    array->data[array->size++] = value; \
} \
\
type get_##name##_DA(name##_DA* array, size_t index) { \
    if(index < array->size) { \
        return array->data[index]; \
    } else { \
        fprintf(stderr, "The index is out of bounds.\n"); \
        return (type)0; \
    } \
} \
\
void remove_##name##_DA(name##_DA *array, size_t index) { \
    if(index < array->size) { \
        memmove(&array->data[index], &array->data[index+1], (array->size - 1 - index) * sizeof(type)); \
        array->size--; \
    } else { \
        fprintf(stderr, "The index is out of bounds.\n"); \
    } \
} \
\
void free_##name##_DA(name##_DA *array) { \
    free(array->data); \
    free(array); \
}

#endif // DSA_H
