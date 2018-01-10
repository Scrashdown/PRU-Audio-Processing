#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "interface.h"

#define OUTFILE "../output/interface.pcm"
#define NSAMPLES 64000 * 1
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

    struct timespec delay = { 0, 250000000 };  // Wait 250 ms
    const size_t limit = 1000;
    enable_recording();
        nanosleep(&delay, NULL);
        for (size_t i = 0; i < limit; ++i) {
            nanosleep(&delay, NULL);
            size_t read = pcm_read(pcm, tmp_buffer, 16500, NCHANNELS);
            fwrite(tmp_buffer, NCHANNELS * SAMPLE_SIZE_BYTES, read, outfile);
            printf("Buffer size : %zu, max = %zu\n", pcm_buffer_length(), pcm_buffer_maxlength());
            printf("Read : %zu/%zu\n", i, limit);
        }
    disable_recording();

    printf("Closing PRU processing...\n");
    fclose(outfile);
    free(tmp_buffer);
    return 0;
}