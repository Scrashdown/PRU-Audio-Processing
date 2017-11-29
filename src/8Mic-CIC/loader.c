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


void processing(FILE * output, volatile uint32_t * PRUmem) {
    uint8_t host_buffer[10 * 16];
    size_t counter = 0;
    for (counter = 0; counter < 10; ++counter) {
        // Wait for PRU interrupt
        printf("Waiting for int...\n");
        prussdrv_pru_wait_event(PRU_EVTOUT_1);
        prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);
        printf("Interrupt received\n");
        // Read 16 bytes from PRU mem
        memcpy((void *) &host_buffer[16 * counter], (const void *) PRUmem, 16);
    }

    // Write the result to the file
    size_t written = fwrite(host_buffer, 16, 10, output);
    if (written != 10) {
        fprintf(stderr, "Error while writing to file!\n");
        fprintf(stderr, "Written = %zu\n", written);
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
    FILE * output = fopen("../output/16bits_8chan.pcm", "w");
    if (output == NULL) {
        fprintf(stderr, "Error! Could not open file (%d)\n", errno);
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
    processing(output, PRUmem);
    
    // Disable PRUs and the pruss driver. Also close the opened file.
    stop(output);

    return 0;
}
