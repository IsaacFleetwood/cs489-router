#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../include/hashmap.h"

void print_word(void* key, size_t size) {
    uint8_t* ptr = (uint8_t*) key;
    printf("0x");
    for(int i = 0; i < size; i++) {
        printf("%02x", ptr[i]);
    }
    printf("\n");
}

hashmap_t hashmap_init(size_t hash_func(void*), size_t size_key, size_t size_value) {
    size_t size_tag_map = (HASHMAP_SIZE_INIT * 2 + 7) / 8; // Round up.
    size_t size_data_map = (size_value + size_key) * HASHMAP_SIZE_INIT;
    hashmap_t hashmap;
    hashmap.hash_func = hash_func;
    hashmap.size_key = size_key;
    hashmap.size_value = size_value;
    hashmap.data = calloc(1, size_tag_map + size_data_map);
    hashmap.size_allocated = HASHMAP_SIZE_INIT;
    hashmap.size = 0;
    return hashmap;
}

size_t hashmap_cyclic_hash(void* ptr, size_t size) {
    uint8_t* byte_ptr = (uint8_t*) ptr;
    size_t ring_size = (size_t) (pow(2, 31) - 1); // a double Mersenne prime. Note: This does cap hashes to 31-bits.
    size_t hash = 0;
    for(int i = 0; i < size; i++) {
        uint8_t val = byte_ptr[i];
        hash = ((hash * 47) ^ val) % ring_size;
    }
    return hash;
}

uint8_t tag_get(hashmap_t* hashmap, size_t index) {
    size_t byte_index = index / 4;
    size_t bit_index = (index % 4) * 2;
    return (hashmap->data[byte_index] >> bit_index) & 0x03;
}

void tag_set(hashmap_t* hashmap, size_t index, uint8_t tag) {
    size_t byte_index = index / 4;
    size_t bit_index = (index % 4) * 2;
    hashmap->data[byte_index] = (hashmap->data[byte_index] & ~(0x03 << bit_index)) | (tag << bit_index);
}

void hashmap_insert(hashmap_t* hashmap, void* key, void* value) {
    size_t hash = hashmap->hash_func(key);
    if(hashmap->size >= hashmap->size_allocated / 2) {
        hashmap_grow(hashmap);
    }
    size_t index = hash % hashmap->size_allocated;
    do {
        uint8_t tag = tag_get(hashmap, index);
        if(tag == HASHMAP_TAG_EMPTY || tag == HASHMAP_TAG_GRAVE) {
            uint8_t* key_ptr = hashmap->data + ((hashmap->size_allocated + 3) / 4) + ((hashmap->size_value + hashmap->size_key) * index);
            uint8_t* value_ptr = key_ptr + hashmap->size_key;
            memcpy(key_ptr, key, hashmap->size_key);
            memcpy(value_ptr, value, hashmap->size_value);
            tag_set(hashmap, index, HASHMAP_TAG_TAKEN);
            hashmap->size += 1;
            break;
        }
        index = (index + 1) % hashmap->size_allocated;
    } while(1);
}

void hashmap_grow(hashmap_t* hashmap) {

    size_t old_size_allocated = hashmap->size_allocated;
    uint8_t* old_tags = hashmap->data;
    uint8_t* old_data = hashmap->data + (hashmap->size_allocated * 2 + 7) / 8;

    size_t size_tag_map = ((hashmap->size_allocated * 2 + 1) * 2 + 7) / 8; // Round up.
    size_t size_data_map = (hashmap->size_value + hashmap->size_key) * (hashmap->size_allocated * 2 + 1);

    hashmap->size_allocated = hashmap->size_allocated * 2 + 1;
    hashmap->data = malloc(size_tag_map + size_data_map);
    hashmap->size = 0;
    uint8_t* tags = hashmap->data;
    uint8_t* data = hashmap->data + size_tag_map;
    for(int i = 0; i < old_size_allocated / 4; i++) {
        uint8_t tag_byte = old_tags[i];
        for(int j = 0; j < 4; j++) {
            uint8_t tag = (tag_byte >> (j * 2)) & 0x03;
            if(tag == HASHMAP_TAG_TAKEN) {
                uint8_t* key_ptr = data + ((hashmap->size_value + hashmap->size_key) + i * 4 + j);
                uint8_t* value_ptr = key_ptr + hashmap->size_key;
                hashmap_insert(hashmap, key_ptr, value_ptr);
            }
        }
    }
    free(old_tags);
}

void* hashmap_get(hashmap_t* hashmap, void* key) {
    size_t hash = hashmap->hash_func(key);
    size_t index = hash % hashmap->size_allocated;
    do {
        uint8_t tag = tag_get(hashmap, index);
        if(tag == HASHMAP_TAG_EMPTY) {
            return NULL;
        } else if(tag == HASHMAP_TAG_TAKEN) {
            uint8_t* key_ptr = hashmap->data + ((hashmap->size_allocated + 3) / 4) + ((hashmap->size_value + hashmap->size_key) * index);
            if(memcmp(key_ptr, key, hashmap->size_key) == 0) {
                uint8_t* value_ptr = key_ptr + hashmap->size_key;
                return value_ptr;
            }
        }
        index = (index + 1) % hashmap->size_allocated;
    } while(1);
}

int hashmap_contains(hashmap_t* hashmap, void* key) {
    return hashmap_get(hashmap, key) != NULL;
}

void hashmap_remove(hashmap_t* hashmap, void* key) {
    size_t hash = hashmap->hash_func(key);
    size_t index = hash % hashmap->size_allocated;
    do {
        uint8_t tag = tag_get(hashmap, index);
        if(tag == HASHMAP_TAG_EMPTY) {
            return;
        } else if(tag == HASHMAP_TAG_TAKEN) {
            uint8_t* key_ptr = hashmap->data + ((hashmap->size_value + hashmap->size_key) + index);
            if(memcmp(key_ptr, key, hashmap->size_key) == 0) {
                tag_set(hashmap, index, HASHMAP_TAG_GRAVE);
                hashmap->size -= 1;
                break;
            }
        }
        index = (index + 1) % hashmap->size_allocated;
    } while(1);
    if(tag_get(hashmap, index + 1) == HASHMAP_TAG_EMPTY) {
        while(tag_get(hashmap, index) == HASHMAP_TAG_GRAVE) {
            tag_set(hashmap, index, HASHMAP_TAG_EMPTY);
            index -= 1;
        }
    }
}