/**
 * Interface of the project.
 * 
 */

#include <stddef.h>
#include "ringbuffer.h"

struct pcm_t {
    // Number of channels
    size_t n_chan;
    // *Per-channel* sample rate of the PCM signal in Hz
    size_t sample_rate;
    // The ring buffer the PCM is bound to
    ringbuffer_t * buffer;
};

/**
 * 
 * 
 */
int pru_processing_init(pcm_t * pcm, size_t n_chan, size_t sample_rate);

/**
 * 
 * 
 */
int pru_processing_close(pcm_t * pcm);

/**
 * Read a given number of blocks of given size and output them to the user provided buffer.
 * 
 */
size_t pcm_read(pcm_t * pcm, void * buffer, size_t blocksize, size_t nblocks);

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