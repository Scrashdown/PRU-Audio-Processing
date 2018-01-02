#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "interface.h"

#define OUTFILE "../output/interface.pcm"

int main(void) {
    printf("\nStarting testing program!\n");

    printf("Open output PCM file...\n");
    FILE * outfile = fopen(OUTFILE, "w");
    if (outfile == NULL) {
        fprintf(stderr, "Error: Could not open output PCM file.\n");
        return 1;
    }

    void * tmp_buffer = calloc(6 * 4, 256000);
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

    int counter = 0;
    struct timespec delay = { 0, 62500 };
    enable_recording();
        while (counter < 256000) {
            nanosleep(&delay, NULL);
            printf("Counter = %i\n", counter);
            counter += pcm_read(pcm, tmp_buffer, 4, 6);
        }
    disable_recording();
    pru_processing_close(pcm);

    printf("Outputting the results to the pcm file...\n");
    fwrite(tmp_buffer, 6 * 4, 256000, outfile);

    printf("Closing PRU processing...\n");
    fclose(outfile);
    free(tmp_buffer);
    return 0;
}