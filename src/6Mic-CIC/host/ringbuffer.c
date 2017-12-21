/**
 * Very simple implementation of a byte ringbuffer.
 * Inspired by : https://embedjournal.com/implementing-circular-buffer-embedded-c/
 * 
 */

#include <stdio.h>
#include <string.h> // For memcpy
#include "ringbuffer.h"

ringbuffer_t * ringbuf_create(size_t nelem, size_t blocksize)
{
    // First, allocate memory for the structure itself
    ringbuffer_t * ringbuf = (ringbuffer_t *) calloc(1, sizeof(ringbuffer_t));
    if (ringbuf == NULL) {
        fprintf(stderr, "Error! Could not allocate memory for ringbuffer.\n");
        return NULL;
    }

    // Then allocate memory for the data buffer of the ring buffer
    uint32_t * data = (uint32_t *) calloc(nelem, blocksize);
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

    return ringbuf;
}


size_t ringbuf_push(ringbuffer_t * dst, uint8_t * data, size_t length)
{
    // Get the distance between the tail and head pointers, and check we aren't trying to push too much data.
    size_t free_bytes;
    if (dst -> head >= dst -> tail) {
        free_bytes = dst -> maxLength - dst -> head + dst -> tail;
    } else {
        free_bytes = dst -> tail - dst -> head;
    }

    // Write only the maximum amount of data possible without overwriting.
    size_t to_write = length > free_bytes ? free_bytes : length;
    memcpy(&(dst -> data[dst -> head]), (const void *) data, to_write);

    // Adjust head pointer
    dst -> head += to_write;
    dst -> head %= dst -> maxLength;
    return to_write;
}


size_t ringbuf_pop(ringbuffer_t * src, uint8_t * data, size_t length)
{
     // If the head isn't ahead of the tail, nothing to read
     // TODO: could implement blocking functionality
     if (src -> head == src -> tail) {
         return -1;
     }

     // Next is where tail will point after reading one byte
     size_t next = (src -> tail + 1) % (src -> maxLength);

     *data = src -> data[src -> tail];
     src -> tail = next;
     return 0;
}


void ringbuf_free(ringbuffer_t * ringbuf)
{
    // First free the ringbuffer's data buffer
    free(ringbuf -> data);
    
    // Then free the data allocated for the ringbuffer itself
    free(ringbuf);
}