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

http://processors.Wiki.ti.com/index.php/PRU_Assembly_Instructions

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

#define OUTPUT_CHAN12 r11

#define INT0_CHAN34 r12
#define INT1_CHAN34 r13
#define INT2_CHAN34 r14
#define INT3_CHAN34 r15
#define LAST_INT_CHAN34 r16

#define COMB0_CHAN34 r17
#define COMB1_CHAN34 r18
#define COMB2_CHAN34 r19
#define LAST_COMB0_CHAN34 r20
#define LAST_COMB1_CHAN34 r21
#define LAST_COMB2_CHAN34 r22

#define OUTPUT_CHAN34 r23

#define IN_PINS r31
#define TMP_REG r29
#define SAMPLE_COUNTER r28

// Input pins offsets
#define CLK_OFFSET 11
#define DAT_OFFSET1 10
#define DAT_OFFSET2 8
#define DAT_OFFSET3 9
#define DAT_OFFSET4 7

// Decimation rate
#define R 16

// Scratchpad register banks numbers
#define BANK0 10
#define BANK1 11
#define BANK2 12
#define PRU1_REGS 14

// DEBUG (assumes P8.11)
#define SET_LED SET r30, r30, 15
#define CLR_LED CLR r30, r30, 15

// Macros for integrator and comb stages
.macro integrators
    // Retrieve data from input pins and perform the 4 first stages of the integrators
    // Stage 0 / 3
    LSR     TMP_REG, IN_PINS, DAT_OFFSET1
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN12.W0, INT0_CHAN12.W0, TMP_REG.W0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET2
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN12.W2, INT0_CHAN12.W2, TMP_REG.W2

    LSR     TMP_REG, IN_PINS, DAT_OFFSET3
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN34.W0, INT0_CHAN34.W0, TMP_REG.W0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET4
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN34.W2, INT0_CHAN34.W2, TMP_REG.W2

    // Perform additional integrator stages, update channels separately
    // Stage 1 / 3
    ADD     INT1_CHAN12.W0, INT1_CHAN12.W0, INT0_CHAN12.W0  // Channel 1
    ADD     INT1_CHAN12.W2, INT1_CHAN12.W2, INT0_CHAN12.W2  // Channel 2
    ADD     INT1_CHAN34.W0, INT1_CHAN34.W0, INT0_CHAN34.W0  // Channel 3
    ADD     INT1_CHAN34.W2, INT1_CHAN34.W2, INT0_CHAN34.W2  // Channel 4
    // Stage 2 / 3
    ADD     INT2_CHAN12.W0, INT2_CHAN12.W0, INT1_CHAN12.W0  // Channel 1
    ADD     INT2_CHAN12.W2, INT2_CHAN12.W2, INT1_CHAN12.W2  // Channel 2
    ADD     INT2_CHAN34.W0, INT2_CHAN34.W0, INT1_CHAN34.W0  // Channel 3
    ADD     INT2_CHAN34.W2, INT2_CHAN34.W2, INT1_CHAN34.W2  // Channel 4
    // Stage 3 / 3
    ADD     INT3_CHAN12.W0, INT3_CHAN12.W0, INT2_CHAN12.W0  // Channel 1
    ADD     INT3_CHAN12.W2, INT3_CHAN12.W2, INT2_CHAN12.W2  // Channel 2
    ADD     INT3_CHAN34.W0, INT3_CHAN34.W0, INT2_CHAN34.W0  // Channel 3
    ADD     INT3_CHAN34.W2, INT3_CHAN34.W2, INT2_CHAN34.W2  // Channel 4
.endm

.macro combs
    // ##### 27 / 72 cycles (at 16kHz)
    // Perform comb stages, update channels separately
    // Stage 0 / 3
    SUB     COMB0_CHAN12.W0, INT3_CHAN12.W0, LAST_INT_CHAN12.W0  // Channel 1
    SUB     COMB0_CHAN12.W2, INT3_CHAN12.W2, LAST_INT_CHAN12.W2  // Channel 2
    SUB     COMB0_CHAN34.W0, INT3_CHAN34.W0, LAST_INT_CHAN34.W0  // Channel 3
    SUB     COMB0_CHAN34.W2, INT3_CHAN34.W2, LAST_INT_CHAN34.W2  // Channel 4
    // Stage 1 / 3
    SUB     COMB1_CHAN12.W0, COMB0_CHAN12.W0, LAST_COMB0_CHAN12.W0  // Channel 1
    SUB     COMB1_CHAN12.W2, COMB0_CHAN12.W2, LAST_COMB0_CHAN12.W2  // Channel 2
    SUB     COMB1_CHAN34.W0, COMB0_CHAN34.W0, LAST_COMB0_CHAN34.W0  // Channel 3
    SUB     COMB1_CHAN34.W2, COMB0_CHAN34.W2, LAST_COMB0_CHAN34.W2  // Channel 4
    // Stage 2 / 3
    SUB     COMB2_CHAN12.W0, COMB1_CHAN12.W0, LAST_COMB1_CHAN12.W0  // Channel 1
    SUB     COMB2_CHAN12.W2, COMB1_CHAN12.W2, LAST_COMB1_CHAN12.W2  // Channel 2
    SUB     COMB2_CHAN34.W0, COMB1_CHAN34.W0, LAST_COMB1_CHAN34.W0  // Channel 3
    SUB     COMB2_CHAN34.W2, COMB1_CHAN34.W2, LAST_COMB1_CHAN34.W2  // Channel 4
    // Stage 3 / 3
    SUB     OUTPUT_CHAN12.W0, COMB2_CHAN12.W0, LAST_COMB2_CHAN12.W0  // Channel 1
    SUB     OUTPUT_CHAN12.W2, COMB2_CHAN12.W2, LAST_COMB2_CHAN12.W2  // Channel 2
    // The result will be written to BANK 0 and taken care of by PRU1
    SUB     OUTPUT_CHAN34.W0, COMB2_CHAN34.W0, LAST_COMB2_CHAN34.W0  // Channel 3
    SUB     OUTPUT_CHAN34.W2, COMB2_CHAN34.W2, LAST_COMB2_CHAN34.W2  // Channel 4
    // The result will be written to BANK 0 and taken care of by PRU1

    // Update comb values
    MOV     LAST_INT_CHAN12, INT3_CHAN12
    MOV     LAST_INT_CHAN34, INT3_CHAN34
    MOV     LAST_COMB0_CHAN12, COMB0_CHAN12
    MOV     LAST_COMB0_CHAN34, COMB0_CHAN34
    MOV     LAST_COMB1_CHAN12, COMB1_CHAN12
    MOV     LAST_COMB1_CHAN34, COMB1_CHAN34
    MOV     LAST_COMB2_CHAN12, COMB2_CHAN12
    MOV     LAST_COMB2_CHAN34, COMB2_CHAN34
.endm

.origin 0
.entrypoint start

start:
    CLR_LED

    // ### Setup start configuration ###
    // Set all register values to zero, except r31
    // Make sure bank 0 is also set to 0
    ZERO    0, 124
    XOUT    BANK0, r0, 124

    // ##### CHANNELS 1 - 4 #####
chan1to4:
    // Swap registers for processing channels 1 - 4
    // Store registers for chan. 5 - 8 in bank 1
    XOUT    BANK1, r0, 96
    // Load registers for chan. 5 - 8 from bank 0
    XIN     BANK0, r0, 96

    // Wait for rising edge
    WBC     IN_PINS, CLK_OFFSET
    WBS     IN_PINS, CLK_OFFSET

    // Wait for t_dv time, since it can be at most 125ns, we have to wait for 25 cycles
    LDI     TMP_REG, 11
wait_data1:
    SUB     TMP_REG, TMP_REG, 1
    QBNE    wait_data1, TMP_REG, 0

    // Update sample counter for decimation
    ADD     SAMPLE_COUNTER, SAMPLE_COUNTER, 1

    // Integrator stages
    integrators  // 24 cycles

    QBNE    chan5to8, SAMPLE_COUNTER, 64

    // Comb stages
    combs  // 24 cycles

    // ##### Channels 5 - 8 #####
chan5to8:
    // Swap registers for processing channels 5 - 8
    // Store registers for chan. 1 - 4 in bank 0
    XOUT    BANK0, r0, 96  // 96B = 24 * 4B = 24 registers that we use to store data for 4 channels
    // Load registers for chan. 5 - 8 from bank 1
    XIN     BANK1, r0, 96  // Same idea

    // Wait for falling edge
    WBS     IN_PINS, CLK_OFFSET
    WBC     IN_PINS, CLK_OFFSET

    // Wait for t_dv time, since it can be at most 125ns, we have to wait for 25 cycles
    LDI     TMP_REG, 11
wait_data2:
    SUB     TMP_REG, TMP_REG, 1
    QBNE    wait_data2, TMP_REG, 0

    // Integrator stages
    integrators  // 24 cycles

    QBNE    chan1to4, SAMPLE_COUNTER, 64
    LDI     SAMPLE_COUNTER, 0  // Reset counter to 0 this time

    // Comb stages...
    combs  // 24 cycles

    // Go back to computing channels 1 - 4
    QBA     chan1to4
