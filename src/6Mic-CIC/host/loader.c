/**
 * @brief Loads the PRU files, executes them, and waits for completion. Headers in loader.h.
 * 
 * @author Loïc Droz <lk.droz@gmail.com>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "loader.h"

#define PRU_NUM0 0
#define PRU_NUM1 1

#define PROGRAM_NAME "pru1.bin"


int setup_mmaps(volatile uint32_t ** pru_mem, volatile void ** host_mem, unsigned int * host_mem_len, unsigned int * host_mem_phys_addr) {
    // Pointer into the PRU1 local data RAM, we use it to send to the PRU the host's memory physical address and length
    volatile void * PRU_mem_void = NULL;
    // For now, store data in 32 bits chunks
    volatile uint32_t * PRU_mem = NULL;
    int ret = prussdrv_map_prumem(PRUSS0_PRU1_DATARAM, (void **) &PRU_mem_void);
    if (ret != 0) {
        return ret;
    }
    PRU_mem = (uint32_t *) PRU_mem_void;

    // Pointer into the DDR RAM mapped by the uio_pruss kernel module, it is possible to change the amount of memory mapped by uio_pruss by reloading it with an argument (still have to find out how)
    volatile void * HOST_mem = NULL;
    ret = prussdrv_map_extmem((void **) &HOST_mem);
    if (ret != 0) {
        return ret;
    }
    unsigned int HOST_mem_len = prussdrv_extmem_size();
    // The PRU needs the physical address of the memory it will write to
    unsigned int HOST_mem_phys_addr = prussdrv_get_phys_addr((void *) HOST_mem);

    // Trim HOST_mem_len down so that it is a multiple of 48. This will ensure that HOST_mem_len / 2 is a multiple of 24 = 6 * 4, which allows the PRU to check if it has reached half if the buffer by doing a simple equality check (24 because it is writing 6 * 4 B at a time)
    HOST_mem_len = HOST_mem_len - (HOST_mem_len % 48);

    printf("%u bytes of Host memory available.\n", HOST_mem_len);
    printf("Physical (PRU-side) address: %x\n", HOST_mem_phys_addr);
    printf("Virtual (Host-side) address: %p\n\n", HOST_mem);

    // Use the first 8 bytes of PRU memory to tell it where the shared segment of Host memory is
    PRU_mem[0] = HOST_mem_phys_addr;
    PRU_mem[1] = HOST_mem_len;

    *pru_mem = PRU_mem;
    *host_mem = HOST_mem;
    *host_mem_len = HOST_mem_len;
    *host_mem_phys_addr = HOST_mem_phys_addr;

    return 0;
}


void stop_program(void) {
    prussdrv_pru_disable(PRU_NUM1);
    prussdrv_exit();
}


int PRU_proc_init(volatile void ** buf, unsigned int * buf_len) {
    // ##### Prussdrv setup #####
    prussdrv_init();
	if (prussdrv_open(PRU_EVTOUT_0) || prussdrv_open(PRU_EVTOUT_1)) {
        fprintf(stderr, "PRU1 : prussdrv_open failed\n");
        return -1;
    }

    // Initialize interrupts or smth like that
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // ##### Setup memory mappings #####
    volatile uint32_t * PRU_mem = NULL;
    unsigned int buf_phys_addr;
    // Setup memory maps and pass the physical address and length of the host's memory to the PRU
    int ret_setup = setup_mmaps(&PRU_mem, buf, buf_len, &buf_phys_addr);
    if (ret_setup != 0) {
        stop_program();
        return -1;
    } else if (PRU_mem == NULL || buf == NULL) {
        stop_program();
        return -1;
    }

    return 0;
}


int load_program(void) {
    // Load the PRU program(s)
    printf("Loading \"%s\" program on PRU1\n", PROGRAM_NAME);
    int ret = prussdrv_exec_program(PRU_NUM1, PROGRAM_NAME);
    if (ret) {
    	fprintf(stderr, "ERROR: could not open %s\n", PROGRAM_NAME);
        stop_program();
    	return ret;
    }
    
    return 0;
}
