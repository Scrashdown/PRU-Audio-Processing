#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "interface.h"

#define OUTFILE "../output/interface.pcm"
#define NSAMPLES 64000
#define NCHANNELS 6

int main(void) {
    printf("\nStarting testing program!\n");

    printf("Open output PCM file...\n");
    FILE * outfile = fopen(OUTFILE, "w");
    if (outfile == NULL) {
        fprintf(stderr, "Error: Could not open output PCM file.\n");
        return 1;
    }

    void * tmp_buffer = calloc(NSAMPLES, NCHANNELS * SAMPLE_SIZE_BYTES);
    if (tmp_buffer == NULL) {
        fprintf(stderr, "Error: Could not allocate enough memory for the testing buffer.\n");
        fclose(outfile);
        return 1;
    }

    printf("Initialize PRU processing...\n");
    pcm_t * pcm = pru_processing_init();
    if (pcm == NULL) {
        free(tmp_buffer);
        fclose(outfile);
        return 1;
    }

    struct timespec delay = { 4, 0 };
    printf("Length of the ringbuffer before recording: %zu (maxLength = %zu)\n", ringbuf_len(pcm -> main_buffer), pcm -> main_buffer -> maxLength);
    enable_recording();
        nanosleep(&delay, NULL);
    disable_recording();
    fflush(stdout);
    printf("Length of the ringbuffer after recording: %zu (maxLength = %zu)\n", ringbuf_len(pcm -> main_buffer), pcm -> main_buffer -> maxLength);

    pcm_read(pcm, tmp_buffer, NSAMPLES, NCHANNELS);
    pru_processing_close(pcm);

    printf("Outputting the results to the pcm file...\n");
    fwrite(tmp_buffer, NCHANNELS * SAMPLE_SIZE_BYTES, NSAMPLES, outfile);

    printf("Closing PRU processing...\n");
    fclose(outfile);
    free(tmp_buffer);
    return 0;
}