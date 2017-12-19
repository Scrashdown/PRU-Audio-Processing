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
    // Flag to enable/disable recording
    int recording_flag;
    // Flag to request stopping of the thread
    int stop_thread_flag;
    // Error status, TODO: necessary ?
    int ret;
} processing_routine_args_t;


// Global variables for threads
pthread_t PRU_thread;

// Arguments for the PRU audio processing thread
processing_routine_args_t args;


// Handles processing the input samples from the PRU, and outputting the results to the ringbuffer
// Also takes care of starting the program.
void *processing_routine(void * __args)
{
    // Load program
    int ret = load_program();
    if (ret) {
        // TODO: exit cleanly
        args.ret = 1;
        pthread_exit((void *) &args);
    }

    // Make sure the size of the PRU buffer is a multiple of 48 as expected
    assert(args.host_datain_buffer_len % 48 == 0);

    int buffer_side = 0;
    volatile void * new_data_start;
    // Process indefinitely
    while (1) {
        // Check if the thread has to terminate
        if (args.stop_thread_flag) {
            args.ret = 0;
            pthread_exit((void *) &args);
        }
        // Wait for PRU interrupt
        prussdrv_pru_wait_event(PRU_EVTOUT_1);
        prussdrv_pru_clear_event(PRU_EVTOUT_1);
        // Swap buffer side to read
        if (buffer_side == 0) {
            new_data_start = args.host_datain_buffer;
            buffer_side = 1;
        } else {
            new_data_start = args.host_datain_buffer + args.host_datain_buffer_len / 2;
            buffer_side = 0;
        }

        // Write the data to the ringbuffer, only if recording is enabled
        if (args.recording_flag) {
            // TODO:
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
    int ret = PRU_proc_init(&host_datain_buffer, &host_datain_buffer_len);
    if (ret) {
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
    
    // Start processing in a separate thread!
    // TODO: May need to pass arguments to the new thread.
    args = {
        .host_datain_buffer = host_datain_buffer,
        .host_datain_buffer_len = host_datain_buffer_len,
        .pcm = pcm,
        .recording_flag = 0, // Do not output to ringbuffer at first
        .ringbuf = ringbuf,
        .ret = 0 };
    ret = pthread_create(&PRU_thread, NULL, processing_routine, NULL);
    if (ret) {
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

    // TODO:

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
    // Disable PRU processing
    stop_program();
    // Then free the pcm ringbuffer
    ringbuf_free(pcm -> main_buffer);
}