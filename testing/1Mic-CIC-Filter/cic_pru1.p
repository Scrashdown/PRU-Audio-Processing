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

.origin 0
.entrypoint TOP

#include "prudefs.hp"

// TODO: Figure out how interrupts work
#define PRU1_ARM_INTERRUPT 20

// Input pins offsets
#define CLK_OFFSET 0
#define DATA_OFFSET 1

// Register aliases
#define IN_PINS r31
#define SAMPLE_COUNTER r4
#define WAIT_COUNTER r5
#define TMP_REG r6

#define INT0 r0
#define INT1 r1
#define INT2 r2
#define INT3 r3
// TODO: define more registers for comb stages

TOP:
    // Setup counters to 0 at first
    LDI     SAMPLE_COUNTER, 0
    // Set all integrator and comb registers to 0 at first
    LDI     INT0, 0
    LDI     INT1, 0
    LDI     INT2, 0
    LDI     INT3, 0
    // TODO: set comb stages to 0 too

WAIT_EDGE:
    // First wait for CLK = 0
    WBC     IN_PINS, CLK_OFFSET
    // Then wait for CLK = 1
    WBS     IN_PINS, CLK_OFFSET

    // Wait for t_dv time, since it can be at most 125ns, we have to wait for 25 cycles
WAIT_SIGNAL:
    LDI     WAIT_COUNTER, 8 // Because 8 = ceil(25 / 3) and the loop takes 3 one-cycle ops
    SUB     WAIT_COUNTER, WAIT_COUNTER, 1
    QBNE    WAIT_SIGNAL, WAIT_COUNTER, 0

    // Retrieve data from DATA pin (only one bit!)
    AND     TMP_REG, IN_PINS, 1 << DATA_OFFSET
    // Do the integrator operations
    ADD     SAMPLE_COUNTER, SAMPLE_COUNTER, 1
    ADD     INT0, INT0, TMP_REG
    ADD     INT1, INT1, INT0
    ADD     INT2, INT2, INT1
    ADD     INT3, INT3, INT2

    // Branch for oversampling
    QBNE    WAIT_EDGE, SAMPLE_COUNTER, 64

    // Reset sample counter once we reach R
    LDI     SAMPLE_COUNTER, 0

    // TODO: comb filter

    // Branch back to wait edge
    QBA     WAIT_EDGE

    // Interrupt the host so it knows we're done
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16

    HALT
