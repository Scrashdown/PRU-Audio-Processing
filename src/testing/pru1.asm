/* Code for the CIC filter in PRU1. */

/* PSEUDOCODE:

Wait for falling (rising ?) edge...
Wait for t_dv time (max 125ns, so 25 cycles)...
Read data from the DATA signal.

counter += 1
r0 += data   // XXX: why not just equals ?
r1 += r0
r2 += r1
r3 += r2

if counter % 64 == 0:
    // TODO

*/

/* Instructions set and macros:

http://processors.wiki.ti.com/index.php/PRU_Assembly_Instructions

*/

#include "prudefs.hasm"

// DEBUG (assumes P8.45)
#define SET_LED SET r30, r30, 0
#define CLR_LED CLR r30, r30, 0

// Scratchpad register banks numbers
#define BANK0 10
#define BANK1 11
#define BANK2 12

#define PRU1_ARM_INTERRUPT 20

.origin 0
.entrypoint start

start:
    /**
     * Check if the XCHG instruction works.
     * Load all zeros in PRUs registers, and all 1s in BANK0.
     * Output the registers of PRU1 to memory before and after XCHG.
     */

    // Memory address
    LDI     r29, 0

    // Fill all registers with 1's
    FILL    0, 29 * 4

    // Export these registers to BANK0
    XOUT    BANK0, r0, 29 * 4

    // Fill all registers with 0's
    ZERO    0, 29 * 4

    // Output current registers to memory
    SBBO    r0, r29, 0, 29 * 4

    // Exchange current registers with BANK0
    XCHG    BANK0, r0, 29 * 4

    // Output exchanged registers to memory
    SBBO    r0, r29, 29 * 4, 29 * 4

    // Exchange again
    XCHG    BANK0, r0, 29 * 4

    // Output again to check if the data was preserved, so if it is actually different from XIN
    SBBO    r0, r29, 2 * 29 * 4, 29 * 4

    // Interrupt the host and finish
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16
    HALT