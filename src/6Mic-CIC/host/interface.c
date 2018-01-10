#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "interface.h"
#include "loader.h"

#define MIN_NCHAN 1
#define MAX_NCHAN 6

#define SUB_BUF_NB 50

typedef struct {
    // Pointer to the PCM signal itself
    pcm_t * pcm;
    // Flag to enable/disable recording
    volatile int recording_flag;
    // Flag to request stopping of the thread
    volatile int stop_thread_flag;
} processing_routine_args_t;


// Global variables for threads
pthread_t PRU_thread;
pthread_attr_t PRU_thread_attr;
pthread_mutex_t ringbuf_mutex = PTHREAD_MUTEX_INITIALIZER;
processing_routine_args_t args;


// Handles processing the input samples from the PRU, and outputting the results to the ringbuffer
// Also takes care of starting the program.
void *processing_routine(void * __args)
{
    // Load program
    if (load_program()) {
        // Disable PRU processing
        pthread_exit(&args);
    }

    int next_evt = PRU_EVTOUT_0;
    volatile void * new_data_start;
    int overflow_flag;

    volatile void * buffer_beginning = args.pcm -> PRU_buffer;
    volatile void * buffer_middle = &(((uint8_t *) args.pcm -> PRU_buffer)[args.pcm -> PRU_buffer_len / 2]);

    // Process indefinitely
    while (1) {
        prussdrv_pru_wait_event(next_evt);
        if (next_evt == PRU_EVTOUT_0) {
            // Even though we are using PRU1, I have to use PRU0_ARM_INTERRUPT in this case for it to work.
            // I truly have no clue of why this is happening.
            prussdrv_pru_clear_event(next_evt, PRU0_ARM_INTERRUPT);
            next_evt = PRU_EVTOUT_1;
            new_data_start = buffer_beginning;
        } else {
            prussdrv_pru_clear_event(next_evt, PRU1_ARM_INTERRUPT);
            next_evt = PRU_EVTOUT_0;
            new_data_start = buffer_middle;
        }

        // Write the data to the ringbuffer, only if recording is enabled
        if (args.recording_flag) {
            // Size of one 6-channel sample tuple, in bytes
            const size_t block_size = SAMPLE_SIZE_BYTES * (args.pcm -> nchan);
            // Number of these blocks to retrieve, must correspond to half of the PRU buffer length
            const size_t block_count = (args.pcm -> PRU_buffer_len) / block_size / 2;
            pthread_mutex_lock(&ringbuf_mutex);
            // Write data to the ringbuffer
            ringbuf_push(args.pcm -> main_buffer, (uint8_t *) new_data_start, block_size, block_count, &overflow_flag);
            pthread_mutex_unlock(&ringbuf_mutex);

            if (overflow_flag) {
                // TODO: Output a warning of some sort
                fprintf(stderr, "Warning! Buffer overflow, some samples have been overwritten.\n");
            }
        }

        // Check if the thread has to terminate
        if (args.stop_thread_flag) {
            // Disable PRU processing
            stop_program();
            pthread_exit((void *) &args);
        }
    }
}


// TODO: add the possibility of adding a filter between the PRU buffer and and the main buffer
pcm_t * pru_processing_init(void)
{
    // Allocate memory for the PCM
    pcm_t * pcm = calloc(1, sizeof(pcm_t));
    if (pcm == NULL) {
        fprintf(stderr, "Error! Memory for pcm could not be allocated.\n");
        return NULL;
    }

    // Initialize memory mappings to get PRU buffer address and length
    if (PRU_proc_init(&(pcm -> PRU_buffer), &(pcm -> PRU_buffer_len))) {
        free(pcm);
        return NULL;
    }

    // Initialize ringbuffer
    ringbuffer_t * ringbuf = ringbuf_create(pcm -> PRU_buffer_len, SUB_BUF_NB);
    if (ringbuf == NULL) {
        free(pcm);
        return NULL;
    }

    // Initialize PCM parameters
    /* TODO: For now, we have a fixed number of channels and sample rate on the PRU. This could be changed in the future.
    However, for now because it is fixed, it makes no sense to allow the user to set these. */
    //pcm -> nchan = nchan;
    //pcm -> sample_rate = sample_rate;
    pcm -> nchan = 6;
    pcm -> sample_rate = 64000;
    pcm -> main_buffer = ringbuf;

    args.pcm = pcm;
    args.recording_flag = 0; // Do not output to ringbuffer at first
    args.stop_thread_flag = 0;

    // Start processing in a separate thread!
    if (pthread_create(&PRU_thread, &PRU_thread_attr, processing_routine, NULL)) {
        fprintf(stderr, "Error! Audio capture thread could not be created.\n");
        pthread_attr_destroy(&PRU_thread_attr);
        ringbuf_free(ringbuf);
        free(pcm);
        return NULL;
    }

    // TODO: maybe wait for some time until the ringbuffer has some samples

    return pcm;
}


size_t pcm_read(pcm_t * src, void * dst, size_t nsamples, size_t nchan)
{
    // First check that the number of channels selected is valid
    if (nchan > src -> nchan) {
        fprintf(stderr, "Error! Specified number of channels is greater than the pcm number of channels.\n");
        return 0;
    }

    // Extract raw data to temporary buffer
    const size_t block_size = SAMPLE_SIZE_BYTES * (args.pcm -> nchan);
    // TODO: use stack or heap for this ?
    uint8_t * raw_data = calloc(nsamples, block_size);
    if (raw_data == NULL) {
        fprintf(stderr, "Error! Could not allocate temporary buffer for raw data.\n");
        return 0;
    }

    pthread_mutex_lock(&ringbuf_mutex);
    // Read data from the ringbuffer
    const size_t read = ringbuf_pop(args.pcm -> main_buffer, raw_data, block_size, nsamples);
    pthread_mutex_unlock(&ringbuf_mutex);

    if (read != nsamples) {
        fprintf(stderr, "Warning! Buffer underflow, some samples could not be read. Expected: %zu, actual: %zu\n", nsamples, read);
    }

    // Extract only the channels we are interested in, and apply some filter
    uint8_t * dst_bytes = (uint8_t *) dst;
    for (size_t s = 0; s < read; ++s) {
        // Only extract the first nchan channels
        memcpy(&dst_bytes[SAMPLE_SIZE_BYTES * nchan * s], &raw_data[block_size * s], SAMPLE_SIZE_BYTES * nchan);
    }

    // TODO: filter

    free(raw_data);
    return read;
}


size_t pcm_buffer_length(void)
{
    pthread_mutex_lock(&ringbuf_mutex);
    size_t length = ringbuf_len(args.pcm -> main_buffer);
    pthread_mutex_unlock(&ringbuf_mutex);
    return length;
}


size_t pcm_buffer_maxlength(void)
{
    return args.pcm -> main_buffer -> maxLength;
}


// Enable writing the PRU samples to the ringbuffer
void enable_recording(void)
{
    args.recording_flag = 1;
}


// Disable writing the PRU samples to the ringbuffer
void disable_recording(void)
{
    args.recording_flag = 0;
}


void pru_processing_close(pcm_t * pcm)
{
    // Stop PRU processing thread
    args.stop_thread_flag = 1;
    // Destroy its attribute
    pthread_attr_destroy(&PRU_thread_attr);
    // Then free the pcm ringbuffer
    ringbuf_free(pcm -> main_buffer);
}