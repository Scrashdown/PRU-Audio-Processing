/**
 * Code for the CIC Filter on PRU0.
 * 
 * Pseudocode :
 * 
 * Wait for rising edge...
 * Wait for t_dv...
 * 
 * Retrieve 4 samples
 * Run CIC filter on 4 channels
 * 
 * */



/* Instructions set and macros:

http://processors.wiki.ti.com/index.php/PRU_Assembly_Instructions

*/

#include "prudefs.hasm"

// Register aliases
#define INT0_CHAN12 r0
#define INT1_CHAN12 r1
#define INT2_CHAN12 r2
#define INT3_CHAN12 r3
#define LAST_INT_CHAN12 r4

#define COMB0_CHAN12 r5
#define COMB1_CHAN12 r6
#define COMB2_CHAN12 r7
#define LAST_COMB0_CHAN12 r8
#define LAST_COMB1_CHAN12 r9
#define LAST_COMB2_CHAN12 r10

#define INT0_CHAN34 r11
#define INT1_CHAN34 r12
#define INT2_CHAN34 r13
#define INT3_CHAN34 r14
#define LAST_INT_CHAN34 r15

#define COMB0_CHAN34 r16
#define COMB1_CHAN34 r17
#define COMB2_CHAN34 r18
#define LAST_COMB0_CHAN34 r19
#define LAST_COMB1_CHAN34 r20
#define LAST_COMB2_CHAN34 r21

#define IN_PINS r31
#define TMP_REG r29
#define SAMPLE_COUNTER r28

// Input pins offsets
#define CLK_OFFSET ???   // TODO:
#define DAT_OFFSET1 ???  // TODO:
#define DAT_OFFSET2 ???  // TODO:
#define DAT_OFFSET3 ???  // TODO:
#define DAT_OFFSET4 ???  // TODO:
#define DAT_OFFSET5 ???  // TODO:
#define DAT_OFFSET6 ???  // TODO:
#define DAT_OFFSET7 ???  // TODO:
#define DAT_OFFSET8 ???  // TODO:

// DEBUG (assumes P8.11)
#define SET_LED SET r30, r30, 15
#define CLR_LED CLR r30, r30, 15

.origin 0
.entrypoint start

start:
    CLR_LED

    // ### Setup start configuration ###
    // Set all register values to zero, except r31
    ZERO    0, 124

wait_rising_edge:
    // First wait for CLK = 0, then wait for CLK = 1
    WBC     IN_PINS, CLK_OFFSET
    WBS     IN_PINS, CLK_OFFSET

    // Wait for t_dv time, since it can be at most 125ns, we have to wait for 25 cycles
    LDI     TMP_REG, 12
wait_data:
    SUB     TMP_REG, TMP_REG, 1
    QBNE    wait_data, TMP_REG, 0

    // Retrieve data from input pins and perform first stage of the integrators
    LSR     TMP_REG, IN_PINS, DAT_OFFSET1
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN12.w0, INT0_CHAN12.w0, TMP_REG.w0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET2
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN12.w2, INT0_CHAN12.w2, TMP_REG.w2

    LSR     TMP_REG, IN_PINS, DAT_OFFSET3
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN34.w0, INT0_CHAN34.w0, TMP_REG.w0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET4
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN34.w2, INT0_CHAN34.w2, TMP_REG.w2

    
