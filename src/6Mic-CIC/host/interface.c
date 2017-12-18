#include <stdio.h>
#include <stdlib.h>
#include "interface.h"
#include "ringbuffer.h"
#include "loader.h"

// TODO: change these values
#define MIN_SAMPLE_RATE_HZ 16000
#define MAX_SAMPLE_RATE_HZ 16000

#define MIN_NCHAN 1
#define MAX_NCHAN 6

#define SUB_BUF_NB 10


pcm_t * pru_processing_init(size_t nchan, size_t sample_rate)
{
    // Check if number of channels makes sense
    if (nchan < MIN_NCHAN || nchan > MAX_NCHAN) {
        fprintf(stderr, "Error! Number of channels must between 1 and 6 (included), actual = %zu\n", nchan);
        return NULL;
    }

    // Check if sample rate is valid
    if (sample_rate < MIN_SAMPLE_RATE_HZ || sample_rate > MAX_SAMPLE_RATE_HZ) {
        fprintf(stderr, 
            "Error! Sample rate in Hz must be between %zu and %zu (included), actual = %zu\n", 
            MIN_SAMPLE_RATE_HZ, MAX_SAMPLE_RATE_HZ, sample_rate);
        return NULL;
    }

    // Allocate memory for the PCM
    pcm_t * pcm = (pcm_t *) calloc(1, sizeof(pcm_t));
    if (pcm == NULL) {
        fprintf(stderr, "Error! Memory for pcm could not be allocated.\n");
        return NULL;
    }

    // Load CIC program on the PRU
    volatile void * host_datain_buffer = NULL;
    size_t host_datain_buffer_len = 0;
    int ret = load_program(&host_datain_buffer, &host_datain_buffer_len);
    if (ret) {
        free((void *) pcm);
        return NULL;
    }

     // Initialize ringbuffer, TODO: give it arguments!
    ringbuffer_t * ringbuf = ringbuf_create(host_datain_buffer_len, SUB_BUF_NB);
    if (ringbuf == NULL) {
        free((void *) pcm);
        return NULL;
    }

    // Initialize PCM parameters
    pcm -> nchan = nchan;
    pcm -> sample_rate = sample_rate;
    pcm -> PRU_buffer = host_datain_buffer;
    pcm -> PRU_buffer_len = host_datain_buffer_len;
    pcm -> main_buffer = ringbuf;
    return pcm;
}


size_t pcm_read(pcm_t * src, void * dst, size_t nsamples, size_t nchan)
{
    
}


void pru_processing_close(pcm_t * pcm)
{
    // First disable PRU processing
    stop_program();

    // Then free the pcm ringbuffer
    ringbuf_free(pcm -> main_buffer);
}