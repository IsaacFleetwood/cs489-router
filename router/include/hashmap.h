#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef struct {
    size_t (*hash_func)(void*);
    size_t size;
    size_t size_allocated;
    size_t size_value;
    size_t size_key;
    uint8_t* data;
    pthread_mutex_t lock_;

} hashmap_t;

#define HASHMAP_SIZE_INIT (11)

#define HASHMAP_TAG_EMPTY (0)
#define HASHMAP_TAG_TAKEN (1)
#define HASHMAP_TAG_GRAVE (2)

size_t hashmap_cyclic_hash(void*, size_t);

hashmap_t hashmap_init(size_t (void*), size_t, size_t);
void hashmap_insert(hashmap_t* hashmap, void* key, void* value);
void hashmap_grow(hashmap_t*);

void* hashmap_get(hashmap_t* hashmap, void* key);
int hashmap_contains(hashmap_t* hashmap, void* key);
void hashmap_remove(hashmap_t* hashmap, void* key);

void hashmap_lock(hashmap_t* hashmap);

void hashmap_unlock(hashmap_t* hashmap);

#endif