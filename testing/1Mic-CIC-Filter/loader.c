// Loads the PRU files, executes them, and waits for completion.
//
// Usage:
// $ ./loader pru0.bin pru1.bin
//
// Compile with:
// gcc -o loader loader.c -lprussdrv
//
// Based on https://credentiality2.blogspot.com/2015/09/beaglebone-pru-gpio-example.html

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pruss/prussdrv.h>
#include <pruss/pruss_intc_mapping.h>

#define PRU_NUM0 0
#define PRU_NUM1 1


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

    // Trim HOST_mem_len down so that it is a multiple of 8. This will ensure that HOST_mem_len / 2 is a multiple of 4, which allows the PRU to check if it has reached half if the buffer by doing a simple equality check (4 because it is writing 4 B at a time)
    HOST_mem_len = (HOST_mem_len >> 3) << 3;

    printf("%u bytes of Host memory available.\n", HOST_mem_phys_addr);
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


void stop(void) {
    prussdrv_pru_disable(PRU_NUM0);
    prussdrv_pru_disable(PRU_NUM1);
    prussdrv_exit();
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s pru0.bin pru1.bin\n", argv[0]);
        return 1;
    }

    prussdrv_init();
    // Load the firmware on both PRUs
    unsigned int ret = prussdrv_open(PRU_EVTOUT_0);
	if (ret) {
        fprintf(stderr, "PRU0 : prussdrv_open failed\n");
        return ret;
    }
    ret = prussdrv_open(PRU_EVTOUT_1);
    if (ret) {
        fprintf(stderr, "PRU1 : prussdrv_open failed\n");
        return ret;
    }

    // Initialize interrupts or smth like that
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // 
    volatile uint32_t * PRU_mem = NULL;
    volatile void * HOST_mem = NULL;
    unsigned int HOST_mem_len = 0;
    unsigned int HOST_mem_phys_addr = 0;
    // Setup memory maps and pass the physical address and length of the host's memory to the PRU
    int ret_setup = setup_mmaps(&PRU_mem, &HOST_mem, &HOST_mem_len, &HOST_mem_phys_addr);
    if (ret_setup != 0) {
        stop();
        return -1;
    }
    
    printf("%u bytes of shared DDR available.\n Physical (PRU-side) address:%x\n",
    HOST_mem_len, HOST_mem_phys_addr);
    printf("Virtual (linux-side) address: %p\n\n", HOST_mem);

    // Load the two PRU programs
    printf("Loading \"%s\" program on PRU0\n", argv[1]);
    ret = prussdrv_exec_program(PRU_NUM0, argv[1]);
    if (ret) {
    	fprintf(stderr, "ERROR: could not open %s\n", argv[1]);
    	return ret;
    }
    printf("Loading \"%s\" program on PRU1\n", argv[2]);
    ret = prussdrv_exec_program(PRU_NUM1, argv[2]);
    if (ret) {
    	fprintf(stderr, "ERROR: could not open %s\n", argv[2]);
    	return ret;
    }

    // TODO : do some checks or or processing on the received data

    // Wait for PRUs to terminate
    prussdrv_pru_wait_event(PRU_EVTOUT_0);
    //prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
    printf("PRU0 program terminated\n");
    prussdrv_pru_wait_event(PRU_EVTOUT_1);
    //prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);
    printf("PRU1 program terminated\n");

    // Disable PRUs and the pruss driver
    stop();

    return 0;
}
