/**
 * Ringbuffer of buffers, each of a constant size.
 * 
 */

#include <stdlib.h>
#include <inttypes.h>

typedef struct {
    // Pointer to the main buffer
    uint32_t * data;
    // Head and tail indices
    size_t head;
    size_t tail;
    // Max length of the buffer
    size_t maxLength;
} ringbuffer_t;

/**
 * 
 * 
 */
ringbuffer_t * ringbuf_create(size_t sub_buf_len, size_t sub_buf_nb);

/**
 * 
 * 
 */
void ringbuf_free(ringbuffer_t * ringbuf);

/**
 * Push one byte
 * 
 */
int ringbuf_push_data(ringbuffer_t * dst, uint8_t data);

/**
 * Pop a given number of elemnts. TODO: decide what elements are.
 * 
 */
int ringbuf_pop(ringbuffer_t * src, uint8_t * data);