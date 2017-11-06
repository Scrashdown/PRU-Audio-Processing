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
#include <pruss/prussdrv.h>
#include <pruss/pruss_intc_mapping.h>

#define PRU_NUM0 0
#define PRU_NUM1 1

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

    // TODO : memory management

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
    prussdrv_pru_disable(PRU_NUM0);
    prussdrv_pru_disable(PRU_NUM1);
    prussdrv_exit();

    return 0;
}
