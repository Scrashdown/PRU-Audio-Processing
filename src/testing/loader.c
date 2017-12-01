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

#define PRU_NUM0 0
#define PRU_NUM1 1


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
    prussdrv_pru_disable(PRU_NUM1);
    prussdrv_exit();

    if (file != NULL) {
        fclose(file);
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

    volatile uint32_t * PRUmem = NULL;
    setup_mmaps(&PRUmem);

    // Load the PRU program(s)
    printf("Loading \"%s\" program on PRU1\n", argv[1]);
    ret = prussdrv_exec_program(PRU_NUM1, argv[1]);
    if (ret) {
    	fprintf(stderr, "ERROR: could not open %s\n", argv[1]);
        stop(NULL);
    	return ret;
    }

    // Wait for PRU interrupt...
    prussdrv_pru_wait_event(PRU_EVTOUT_1);
    prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);

    // Display memory contents.
    printf("PRU1 registers before XCHG:\n");
    for (size_t i = 0; i < 29; ++i) {
        uint32_t value = PRUmem[i];
        printf("\tR%zu:\t0x%.8x\n", i, value);
    }
    printf("\nPRU1 registers after first XCHG:\n");
    for (size_t i = 0; i < 29; ++i) {
        uint32_t value = PRUmem[29 + i];
        printf("\tR%zu:\t0x%.8x\n", i, value);
    }
    printf("\nPRU1 registers after second XCHG:\n");
    for (size_t i = 0; i < 29; ++i) {
        uint32_t value = PRUmem[2 * 29 + i];
        printf("\tR%zu:\t0x%.8x\n", i, value);
    }
    
    // Disable PRUs and the pruss driver. Also close the opened file.
    stop(NULL);

    return 0;
}
