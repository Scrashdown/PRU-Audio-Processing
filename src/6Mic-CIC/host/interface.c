#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "interface.h"
#include "ringbuffer.h"
#include "loader.h"

// TODO: change these values
#define MIN_SAMPLE_RATE_HZ 16000
#define MAX_SAMPLE_RATE_HZ 16000

#define MIN_NCHAN 1
#define MAX_NCHAN 6

#define SUB_BUF_NB 10

#define SAMPLE_SIZE_BYTES 4

typedef struct {
    // Virtual address of the buffer the PRU writes to
    volatile void * host_datain_buffer;
    // Length of the aforementioned buffer
    unsigned int host_datain_buffer_len;
    // Buffer to which output the data
    ringbuffer_t * ringbuf;
    // Pointer to the PCM signal itself
    pcm_t * pcm;
    // TODO: function pointer to an optional filter

    // Flag to enable/disable recording
    int recording_flag;
    // Flag to request stopping of the thread
    int stop_thread_flag;
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
    // Make sure the size of the PRU buffer is a multiple of 48 as expected
    int next_evt = PRU_EVTOUT_0;
    volatile void * new_data_start;
    // Load program
    if (load_program()) {
        // Disable PRU processing
        pthread_exit((void *) &args);
    }

    // Process indefinitely
    while (1) {
        prussdrv_pru_wait_event(next_evt);
        prussdrv_pru_clear_event(next_evt, PRU1_ARM_INTERRUPT);
        
        if (next_evt == PRU_EVTOUT_0) {
            next_evt = PRU_EVTOUT_1;
            new_data_start = args.host_datain_buffer;
        } else {
            next_evt = PRU_EVTOUT_0;
            new_data_start = args.host_datain_buffer + args.host_datain_buffer_len / 2;
        }

        // Write the data to the ringbuffer, only if recording is enabled
        if (args.recording_flag) {
            size_t block_size = 4 * (args.pcm -> nchan);
            size_t block_count = (size_t) args.host_datain_buffer_len / block_size;
            pthread_mutex_lock(&ringbuf_mutex);
            // Write data to the ringbuffer
            size_t written = ringbuf_push(args.ringbuf, (uint8_t *) args.host_datain_buffer, block_size, block_count);
            pthread_mutex_unlock(&ringbuf_mutex);

            if (written != (size_t) args.host_datain_buffer) {
                // TODO: Output a warning of some sort
                fprintf(stderr, "Warning! Buffer overflow, some samples could not be written.\n");
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

    // Initialize memory mappings to get PRU buffer address and length
    volatile void * host_datain_buffer;
    unsigned int host_datain_buffer_len;
    if (PRU_proc_init(&host_datain_buffer, &host_datain_buffer_len)) {
        free((void *) pcm);
        return NULL;
    }

    // Initialize ringbuffer
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

    args = {
        .host_datain_buffer = host_datain_buffer,
        .host_datain_buffer_len = host_datain_buffer_len,
        .pcm = pcm,
        .recording_flag = 0, // Do not output to ringbuffer at first
        .ringbuf = ringbuf };

    // Initialize thread parameters
    if (pthread_attr_init(&PRU_thread_attr) || pthread_attr_setschedparam(&PRU_thread_attr, SCHED_RR)) {
        fprintf(stderr, "Error! Could not create thread attribute.\n");
        ringbuf_free(ringbuf);
        free((void *) pcm);
        return NULL;
    }
    
    // Start processing in a separate thread!
    if (pthread_create(&PRU_thread, &PRU_thread_attr, processing_routine, NULL)) {
        fprintf(stderr, "Error! Audio capture thread could not be created.\n");
        pthread_attr_destroy(&PRU_thread_attr);
        ringbuf_free(ringbuf);
        free((void *) pcm);
        return NULL;
    }

    // TODO: maybe wait for some time until the ringbuffer has some samples

    return pcm;
}

// TODO: rework
int pcm_read(pcm_t * src, void * dst, size_t nsamples, size_t nchan)
{
    // First check that the number of channels selected is valid
    if (nchan > src -> nchan) {
        return -1;
    }

    size_t byte_length = nsamples * nchan * 4;
    pthread_mutex_lock(&ringbuf_mutex);
    // Read data to the ringbuffer
    size_t read = ringbuf_pop(args.ringbuf, (uint8_t *) dst, byte_length);
    pthread_mutex_unlock(&ringbuf_mutex);

    if (read != byte_length) {
        // TODO: Output a warning of some sort
        fprintf(stderr, "Warning! Buffer overflow, some samples could not be read.\n");
    }

    return 0;
}


// Enable writing the PRU samples to the ringbuffer
int enable_recording()
{
    args.recording_flag = 1;
}


// Disable writing the PRU samples to the ringbuffer
void disable_recording()
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