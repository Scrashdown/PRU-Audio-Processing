#include <stdio.h>
#include "ringbuffer.h"

ringbuffer_t * ringbuf_create(size_t sub_buf_len, size_t sub_buf_nb)
{
    // First, allocate memory for the structure itself
    ringbuffer_t * ringbuf = (ringbuffer_t *) calloc(1, sizeof(ringbuffer_t));
    if (ringbuf == NULL) {
        fprintf(stderr, "Error! Could not allocate memory for ringbuffer.\n");
        return NULL;
    }

    // Then allocate memory for the data buffer of the ring buffer
    uint32_t * data = (uint32_t *) calloc(sub_buf_nb, sub_buf_len);
    if (data == NULL) {
        fprintf(stderr, "Error! Could not allocate memory for ringbuffer's data buffer.\n");
        free(ringbuf);
        return NULL;
    }

    // Finally, set the ringbuffer's parameters
    ringbuf -> data = data;
    ringbuf -> head = 0;
    ringbuf -> tail = 0;
    ringbuf -> maxLength = sub_buf_nb * sub_buf_len;

    return ringbuf;
}


void ringbuf_free(ringbuffer_t * ringbuf)
{
    // First free the ringbuffer's data buffer
    free(ringbuf -> data);
    
    // Then free the data allocated for the ringbuffer itself
    free(ringbuf);
}