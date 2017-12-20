/**
 * Very simple implementation of a byte ringbuffer.
 * Inspired by : https://embedjournal.com/implementing-circular-buffer-embedded-c/
 * 
 */

#include <stdio.h>
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
    // Next is where the head pointer will point after writing one byte
    size_t next = (dst -> head + 1) % (dst -> maxLength);

    // Check if the buffer and return an error if it is
    if (next == dst -> tail) {
        return -1;
    }

    dst -> data[dst -> head] = data;
    dst -> head = next;
    return 0;
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