/**
 * Interface of the project.
 * 
 */

#include <stddef.h>
#include "ringbuffer.h"

typedef struct pcm_t {
    // Number of channels
    size_t nchan;
    // *Per-channel* sample rate of the PCM signal in Hz
    size_t sample_rate;
    // The buffer accessed by the PRU, mapped by prussdrv, and its length
    volatile void * PRU_buffer;
    size_t PRU_buffer_len;
    // The ring buffer which the main place for storing data
    ringbuffer_t * main_buffer;
    // Function pointer to an optional filter
    // TODO:
} pcm_t;

/**
 * 
 * 
 */
pcm_t * pru_processing_init(size_t nchan, size_t sample_rate);

/**
 * 
 * 
 */
void pru_processing_close(pcm_t * pcm);

/**
 * Read a given number of blocks of given size and output them to the user provided buffer.
 * 
 */
int pcm_read(pcm_t * src, void * dst, size_t nsamples, size_t nchan);

/**
 * 
 * 
 */
int enable_recording();

/**
 * 
 * 
 */
void disable_recording();