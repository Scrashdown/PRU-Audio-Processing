/**
 * @brief Simple implementation of a single-threaded ringbuffer/queue.
 * 
 */

#include <stdlib.h>
#include <inttypes.h>

typedef struct {
    // Pointer to the main buffer
    uint8_t * data;
    // Head and tail indices
    size_t head;
    size_t tail;
    // Max length of the buffer
    size_t maxLength;
    // Flag for checking if the buffer is full
    // Needed because if the buffer is full or empty, the head and tail indexes will be the same
    int is_full;
} ringbuffer_t;

/**
 * @brief Create a new ringbuffer containing the given number of blocks of given length.
 * 
 * @param nelem Number of blocks.
 * @param blocksize Block size.
 * @return ringbuffer_t* A pointer to a new ringbuffer in case of success, NULL otherwise.
 */
ringbuffer_t * ringbuf_create(size_t nelem, size_t blocksize);

/**
 * @brief Free the resources allocated for the given ringbuffer.
 * 
 * @param ringbuf The ringbuffer to free.
 */
void ringbuf_free(ringbuffer_t * ringbuf);

// TODO: return the number of samples actually written/read

/**
 * @brief Push data to the ringbuffer.
 * 
 * @param dst The ringbuffer to which data must be pushed.
 * @param data The data to push.
 * @param block_size The size of each block of data.
 * @param block_count The number of blocks to push.
 * @return size_t The number of blocks effectively written.
 */
size_t ringbuf_push(ringbuffer_t * dst, uint8_t * data, size_t block_size, size_t block_count);

/**
 * @brief Pop data from the ringbuffer.
 * 
 * @param src The ringbuffer from which data must be popped.
 * @param data The buffer to which output the data. Has to be large enough.
 * @param block_size The size of each block of data.
 * @param block_count The number of blocks to push.
 * @return size_t The number of blocks effectively read.
 */
size_t ringbuf_pop(ringbuffer_t * src, uint8_t * data, size_t block_size, size_t block_count);

/**
 * @brief Get the length of a ringbuffer.
 * 
 * @param buf The ringbuffer of which we seek the length.
 * @return size_t The length of the ringbuffer. 0 if empty, buf -> maxLength if it is full.
 */
size_t ringbuf_len(ringbuffer_t * buf);