/**
 * Very simple implementation of a byte ringbuffer.
 * Inspired by : https://embedjournal.com/implementing-circular-buffer-embedded-c/
 * 
 */

#include <stdio.h>
#include <string.h> // For memcpy
#include <assert.h>
#include "ringbuffer.h"

ringbuffer_t * ringbuf_create(size_t blocksize, size_t nelem)
{
    // First, allocate memory for the structure itself
    ringbuffer_t * ringbuf = calloc(1, sizeof(ringbuffer_t));
    if (ringbuf == NULL) {
        fprintf(stderr, "Error! Could not allocate memory for ringbuffer.\n");
        return NULL;
    }

    // Then allocate memory for the data buffer of the ring buffer
    uint8_t * data = calloc(nelem, blocksize);
    if (data == NULL) {
        fprintf(stderr, "Error! Could not allocate memory for ringbuffer's data buffer.\n");
        free(ringbuf);
        return NULL;
    }

    // Finally, set the ringbuffer's parameters
    ringbuf -> data = data;
    ringbuf -> head = 0;
    ringbuf -> tail = 0;
    ringbuf -> maxLength = nelem * blocksize;
    ringbuf -> is_full = 0;

    return ringbuf;
}


size_t ringbuf_push(ringbuffer_t * dst, uint8_t * data, size_t block_size, size_t block_count, int * overflow_flag)
{
    if (block_size == 0 || block_count == 0) {
        return 0;
    }

    // Get the distance between the tail and head pointers, and check we aren't trying to push too much data.
    size_t free_bytes;
    if (dst -> is_full) {
        free_bytes = 0;
    } else if (dst -> head >= dst -> tail) {
        free_bytes = (dst -> maxLength) - (dst -> head) + (dst -> tail);
    } else {
        free_bytes = (dst -> tail) - (dst -> head);
    }

    size_t to_write = block_size * block_count;
    // Check if an overflow will occur or not.
    *overflow_flag = (free_bytes < to_write) ? 1 : 0;

    // Copy the data
    // DO NOT COPY EVERYTHING AT ONCE, we need to copy in 2 parts if we have to loop back to the beginning of the actual buffer in memory
    if ((dst -> maxLength - dst -> head) > to_write) {
        // Can copy everything at once
        memcpy(&(dst -> data[dst -> head]), data, to_write);
    } else {
        // Need to copy the first half up to the end of the actual buffer in memory from head, then copy the rest to the beginning of the said buffer
        const size_t first_half_len = dst -> maxLength - dst -> head;
        memcpy(&(dst -> data[dst -> head]), data, first_half_len);
        memcpy(dst -> data, &data[first_half_len], to_write - first_half_len);
    }

    // Adjust head pointer
    dst -> head += to_write;
    dst -> head %= dst -> maxLength;
    // In case of an overflow, adjust tail pointer as well
    if (overflow_flag) {
        dst -> tail = dst -> head;
    }
    
    // Check if the buffer is now full
    dst -> is_full = (dst -> head == dst -> tail) ? 1 : 0;
    return to_write / block_size;
}


size_t ringbuf_pop(ringbuffer_t * src, uint8_t * data, size_t block_size, size_t block_count)
{
    // Get the distance between the tail and head pointers, check we aren't trying to pop too much data.
    size_t available_bytes;
    if ((src -> head == src -> tail) && src -> is_full) {
        available_bytes = src -> maxLength;
    } else if (src -> head >= src -> tail) {
        available_bytes = (src -> head) - (src -> tail);
    } else {
        available_bytes = (src -> maxLength) - (src -> tail) + (src -> head);
    }

    // Read only the maximum amount of data possible such that no block is partially read
    size_t to_read = (block_size * block_count) > available_bytes ? available_bytes : (block_size * block_count);
    to_read -= to_read % block_size;
    
    // Copy the data
    // DO NOT COPY EVERYTHING AT ONCE, we might need to copy in 2 parts if we have to loop back to the beginning of the actual buffer in memory
    if ((src -> maxLength - src -> tail) > to_read) {
        memcpy(data, &(src -> data[src -> tail]), to_read);
    } else {
        // Need to copy the first half up to the end of the actual buffer in memory from head, then copy the rest from the beginning of the said buffer
        const size_t first_half_len = src -> maxLength - src -> tail;
        memcpy(data, &(src -> data[src -> tail]), first_half_len);
        memcpy(&data[first_half_len], src -> data, to_read - first_half_len);
    }

    // Adjust tail pointer
    src -> tail += to_read;
    src -> tail %= src -> maxLength;
    src -> is_full = src -> is_full && (to_read == 0);
    return to_read / block_size;
}


void ringbuf_free(ringbuffer_t * ringbuf)
{
    // First free the ringbuffer's data buffer
    free(ringbuf -> data);
    // Then free the data allocated for the ringbuffer itself
    free(ringbuf);
}


size_t ringbuf_len(ringbuffer_t * buf)
{
    if (buf -> is_full) {
        return buf -> maxLength;
    }
    
    const size_t head = buf -> head;
    const size_t tail = buf -> tail;

    if (head >= tail) {
        return head - tail;
    } else {
        return head + (buf -> maxLength - tail);
    }
}