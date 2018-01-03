/**
 * Very simple implementation of a byte ringbuffer.
 * Inspired by : https://embedjournal.com/implementing-circular-buffer-embedded-c/
 * 
 */

#include <stdio.h>
#include <string.h> // For memcpy
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


size_t ringbuf_push(ringbuffer_t * dst, uint8_t * data, size_t block_size, size_t block_count)
{
    if (dst -> is_full) {
        return 0;
    }

    // Get the distance between the tail and head pointers, and check we aren't trying to push too much data.
    size_t free_bytes;
    if (dst -> head >= dst -> tail) {
        free_bytes = (dst -> maxLength) - (dst -> head) + (dst -> tail);
    } else {
        free_bytes = (dst -> tail) - (dst -> head);
    }

    // Write only the maximum amount of data possible without overwriting.
    size_t to_write = (block_size * block_count) > free_bytes ? free_bytes : (block_size * block_count);
    // Trim to_write to make sure we do not partially write a set of samples in case of an overflow
    to_write -= to_write % block_size;
    // Copy the data
    // TODO: DO NOT COPY EVERYTHING AT ONCE, we might need to copy in 2 parts if we have to loop back to the beginning of the actual buffer in memory
    memcpy(&(dst -> data[dst -> head]), data, to_write);

    // Adjust head pointer
    dst -> head += to_write;
    dst -> head %= dst -> maxLength;
    dst -> is_full = dst -> head == dst -> tail ? 1 : 0;
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

    // Read only the maximum amount of data possible without underflow.
    size_t to_read = (block_size * block_count) > available_bytes ? available_bytes : (block_size * block_count);
    // Trim to_read to make sure we do not partially pop some block which would cause misalignment.
    to_read -= to_read % block_size;
    // Copy the data
    // TODO: DO NOT COPY EVERYTHING AT ONCE, we might need to copy in 2 parts if we have to loop back to the beginning of the actual buffer in memory
    memcpy(data, &(src -> data[src -> tail]), to_read);

    // Adjust tail pointer
    src -> tail += to_read;
    src -> tail %= src -> maxLength;
    src -> is_full = src -> is_full && !to_read;
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
    
    size_t head = buf -> head;
    size_t tail = buf -> tail;

    if (head >= tail) {
        return head - tail;
    } else {
        return head + (buf -> maxLength - tail);
    }
}