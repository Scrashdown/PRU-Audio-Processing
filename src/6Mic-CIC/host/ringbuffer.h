/**
 * Ringbuffer of buffers, each of a constant size.
 * 
 */

#include <stddef.h>

typedef struct ringbuffer_t {

} ringbuffer_t;

/**
 * TODO:
 * 
 */
ringbuffer_t * ringbuf_create();

/**
 * 
 * 
 */
void ringbuf_free(ringbuffer_t * ringbuf);

/**
 * Push one frame (= 1 half PRU buffer) to the buffer.
 * 
 */
int ringbuf_push_frame(const void * src, ringbuffer_t * dst);

/**
 * Pop a given number of elemnts. TODO: decide what elements are.
 * 
 */
size_t ringbuf_pop(const ringbuffer_t * src, void * dest, size_t nbelem);