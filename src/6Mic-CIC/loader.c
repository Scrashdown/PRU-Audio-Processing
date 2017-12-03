// Loads the PRU files, executes them, and waits for completion.
//
// Usage:
// $ ./loader pru1.bin
//
// Compile with:
// gcc -o loader loader.c -lprussdrv
//
// Based on https://credentiality2.blogspot.com/2015/09/beaglebone-pru-gpio-example.html

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <pruss/prussdrv.h>
#include <pruss/pruss_intc_mapping.h>

#define PRU0 0
#define PRU1 1


int setup_mmaps(volatile uint32_t ** pru_mem) {
    // Pointer into the PRU1 local data RAM, we use it to send to the PRU the host's memory physical address and length
    volatile void * PRU_mem_void = NULL;
    // For now, store data in 32 bits chunks
    volatile uint32_t * PRU_mem = NULL;
    int ret = prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, (void **) &PRU_mem_void);
    if (ret != 0) {
        return ret;
    }
    PRU_mem = (uint32_t *) PRU_mem_void;
    *pru_mem = PRU_mem;
    return 0;
}


void stop(FILE * file) {
    printf("Exiting program...\n");
    prussdrv_pru_disable(PRU1);
    prussdrv_exit();

    if (file != NULL) {
        fclose(file);
    }
}


void processing(FILE * output, const uint8_t * host_buffer, size_t sample_count, volatile uint32_t * PRUmem) {
    for (size_t counter = 0; counter < sample_count; ++counter) {
        // Wait for PRU interrupt
        prussdrv_pru_wait_event(PRU_EVTOUT_1);
        prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);
        // Read 6 * 4 bytes from PRUmem, because each sample holds on 4 bytes
        memcpy((void *) &host_buffer[6 * 4 * counter], (const void *) PRUmem, 6 * 4);
    }

    // Write the result to the file
    size_t written = fwrite(host_buffer, 6 * 4, sample_count, output);
    if (written != sample_count) {
        fprintf(stderr, "Error while writing to file!\n");
        fprintf(stderr, "Written = %zu, expected = %zu\n", written, sample_count);
    }
}


int main(int argc, char ** argv) {
    if (argc != 2) {
        printf("Usage: %s pru1.bin\n", argv[0]);
        return 1;
    }

    // ##### Prussdrv setup #####
    prussdrv_init();
    unsigned int ret = prussdrv_open(PRU_EVTOUT_1);
	if (ret) {
        fprintf(stderr, "PRU1 : prussdrv_open failed\n");
        return ret;
    }

    // Initialize interrupts or smth like that
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // Setup memory mapping
    volatile uint32_t * PRUmem = NULL;
    setup_mmaps(&PRUmem);

    // Open file for output
    FILE * output = fopen("../output/32bits_6chan.pcm", "wb");
    if (output == NULL) {
        fprintf(stderr, "Error! Could not open file (%d)\n", errno);
        stop(output);
        return -1;
    }

    // Allocate host buffer for temporary storage of values
    const size_t sample_size = 6 * 4;  // 6 channels, each 4 bytes
    const size_t sample_count = 200000;
    const uint8_t * host_buffer = (const uint8_t *) calloc(sample_count, sample_size);
    if (host_buffer == NULL) {
        fprintf(stderr, "Error allocating host buffer!\n");
        stop(output);
        return -1;
    }

    // Load the PRU program(s)
    printf("Loading \"%s\" program on PRU1\n", argv[1]);
    ret = prussdrv_exec_program(PRU1, argv[1]);
    if (ret) {
    	fprintf(stderr, "ERROR: could not open %s\n", argv[1]);
        stop(output);
    	return ret;
    }

    printf("Processing...\n");
    // Start processing of the received data
    processing(output, host_buffer, sample_count, PRUmem);
    
    // Disable PRUs and the pruss driver. Also close the opened file.
    stop(output);

    return 0;
}
