/**
 * @brief Very basic interface for reading audio using the PRU firmware.
 * 
 */

#include "ringbuffer.h"
#include "loader.h"


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
 * @brief Initialize PRU processing. Must be called before any other function of this file.
 * 
 * @return pcm_t* A pointer to a new pcm object in case of success, NULL otherwise.
 */
//pcm_t * pru_processing_init(size_t nchan, size_t sample_rate);
pcm_t * pru_processing_init(void);

/**
 * @brief Stop processing and free/close all resources.
 * 
 * @param pcm The pcm object containing the resources.
 */
void pru_processing_close(pcm_t * pcm);

/**
 * @brief Read a given number of blocks of given size and output them to the user provided buffer.
 * 
 * @param src 
 * @param dst 
 * @param nsamples 
 * @param nchan 
 * @return int 
 */
int pcm_read(pcm_t * src, void * dst, size_t nsamples, size_t nchan);

/**
 * @brief Enable recording of the audio to the ringbuffer.
 * 
 * Once this function is called, the interface will start copying
 * from the buffer the PRU writes to, to the main ringbuffer of the interface. In order to avoid
 * a ringbuffer overflow, the user must therefore start reading using pcm_read quickly after this
 * function had been called.
 * 
 */
void enable_recording(void);

/**
 * @brief Disable recording of the audio to the ringbuffer.
 * 
 * Once this function is called, the interface will stop copying data to the main ringbuffer.
 * This means only the samples remaining in the ringbuffer after this function was called can be read.
 * 
 */
void disable_recording(void);