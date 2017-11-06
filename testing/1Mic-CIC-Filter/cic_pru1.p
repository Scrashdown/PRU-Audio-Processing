/* Code for the CIC filter in PRU1. */
.origin 0
.entrypoint TOP

#include "prudefs.hp"

// TODO: Figure out how interrupts work
#define PRU1_ARM_INTERRUPT 20

TOP:

    // Interrupt the host so it knows we're done
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16

    HALT
