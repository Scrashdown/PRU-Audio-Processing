/**
 * Code for the CIC Filter on PRU0.
 * 
 * Instruction set :
 * http://processors.Wiki.ti.com/index.php/PRU_Assembly_Instructions
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

#define OUTPUT_CHAN12 r22
#define OUTPUT_CHAN34 r23

#define IN_PINS r31
#define TMP_REG r29
#define SAMPLE_COUNTER r28
#define LOCAL_MEM r27

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

// Defined in the PRU ref. guide
#define LOCAL_MEM_ADDR 0x0

#define PRU1_ARM_INTERRUPT 20

// DEBUG (assumes P8.45)
#define SET_LED SET r30, r30, 0
#define CLR_LED CLR r30, r30, 0

// Macros for integrator and comb stages
.macro integrators
    // Retrieve data from input pins and perform the 4 first stages of the integrators
    // Stage 0 / 3
    LSR     TMP_REG, IN_PINS, DAT_OFFSET1  // Load data from channel 1 or 5
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN12.W0, INT0_CHAN12.W0, TMP_REG.W0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET2  // Load data from channel 2 or 6
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN12.W2, INT0_CHAN12.W2, TMP_REG.W0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET3  // Load data from channel 3 or 7
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN34.W0, INT0_CHAN34.W0, TMP_REG.W0

    LSR     TMP_REG, IN_PINS, DAT_OFFSET4  // Load data from channel 4 or 8
    AND     TMP_REG, TMP_REG, 1
    ADD     INT0_CHAN34.W2, INT0_CHAN34.W2, TMP_REG.W0

    // Perform additional integrator stages, update channels separately
    // Stage 1 / 3
    ADD     INT1_CHAN12.W0, INT1_CHAN12.W0, INT0_CHAN12.W0  // Channel 1 or 5
    ADD     INT1_CHAN12.W2, INT1_CHAN12.W2, INT0_CHAN12.W2  // Channel 2 or 6
    ADD     INT1_CHAN34.W0, INT1_CHAN34.W0, INT0_CHAN34.W0  // Channel 3 or 7
    ADD     INT1_CHAN34.W2, INT1_CHAN34.W2, INT0_CHAN34.W2  // Channel 4 or 8
    // Stage 2 / 3
    ADD     INT2_CHAN12.W0, INT2_CHAN12.W0, INT1_CHAN12.W0  // Channel 1 or 5
    ADD     INT2_CHAN12.W2, INT2_CHAN12.W2, INT1_CHAN12.W2  // Channel 2 or 6
    ADD     INT2_CHAN34.W0, INT2_CHAN34.W0, INT1_CHAN34.W0  // Channel 3 or 7
    ADD     INT2_CHAN34.W2, INT2_CHAN34.W2, INT1_CHAN34.W2  // Channel 4 or 8
    // Stage 3 / 3
    ADD     INT3_CHAN12.W0, INT3_CHAN12.W0, INT2_CHAN12.W0  // Channel 1 or 5
    ADD     INT3_CHAN12.W2, INT3_CHAN12.W2, INT2_CHAN12.W2  // Channel 2 or 6
    ADD     INT3_CHAN34.W0, INT3_CHAN34.W0, INT2_CHAN34.W0  // Channel 3 or 7
    ADD     INT3_CHAN34.W2, INT3_CHAN34.W2, INT2_CHAN34.W2  // Channel 4 or 8
.endm

.macro combs
.mparam memoffset
    // ##### 27 / 72 cycles (at 16kHz)
    // Perform comb stages, update channels separately
    // Stage 0 / 3
    SUB     COMB0_CHAN12.W0, INT3_CHAN12.W0, LAST_INT_CHAN12.W0  // Channel 1 or 5
    SUB     COMB0_CHAN12.W2, INT3_CHAN12.W2, LAST_INT_CHAN12.W2  // Channel 2 or 6
    SUB     COMB0_CHAN34.W0, INT3_CHAN34.W0, LAST_INT_CHAN34.W0  // Channel 3 or 7
    SUB     COMB0_CHAN34.W2, INT3_CHAN34.W2, LAST_INT_CHAN34.W2  // Channel 4 or 8
    // Stage 1 / 3
    SUB     COMB1_CHAN12.W0, COMB0_CHAN12.W0, LAST_COMB0_CHAN12.W0  // Channel 1 or 5
    SUB     COMB1_CHAN12.W2, COMB0_CHAN12.W2, LAST_COMB0_CHAN12.W2  // Channel 2 or 6
    SUB     COMB1_CHAN34.W0, COMB0_CHAN34.W0, LAST_COMB0_CHAN34.W0  // Channel 3 or 7
    SUB     COMB1_CHAN34.W2, COMB0_CHAN34.W2, LAST_COMB0_CHAN34.W2  // Channel 4 or 8
    // Stage 2 / 3
    SUB     COMB2_CHAN12.W0, COMB1_CHAN12.W0, LAST_COMB1_CHAN12.W0  // Channel 1 or 5
    SUB     COMB2_CHAN12.W2, COMB1_CHAN12.W2, LAST_COMB1_CHAN12.W2  // Channel 2 or 6
    SUB     COMB2_CHAN34.W0, COMB1_CHAN34.W0, LAST_COMB1_CHAN34.W0  // Channel 3 or 7
    SUB     COMB2_CHAN34.W2, COMB1_CHAN34.W2, LAST_COMB1_CHAN34.W2  // Channel 4 or 8
    // Stage 3 / 3
    SUB     OUTPUT_CHAN12.W0, COMB2_CHAN12.W0, LAST_COMB2_CHAN12.W0  // Channel 1 or 5
    SUB     OUTPUT_CHAN12.W2, COMB2_CHAN12.W2, LAST_COMB2_CHAN12.W2  // Channel 2 or 6
    SUB     OUTPUT_CHAN34.W0, COMB2_CHAN34.W0, LAST_COMB2_CHAN34.W0  // Channel 3 or 7
    SUB     OUTPUT_CHAN34.W2, COMB2_CHAN34.W2, LAST_COMB2_CHAN34.W2  // Channel 4 or 8
    // Store result in PRU0 local memory
    // Stores OUTPUT_CHAN12 and the next register, OUTPUT_CHAN34
    SBBO    OUTPUT_CHAN12, LOCAL_MEM, memoffset, 8

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
    ZERO    0, 120
    XOUT    BANK0, r0, 120

    // Load local mem address in a register
    LDI     LOCAL_MEM, LOCAL_MEM_ADDR

    // ##### CHANNELS 1 - 4 #####
chan1to4:
    // Swap registers for processing channels 1 - 4
    // Store registers for chan. 5 - 8 in bank 1
    XOUT    BANK1, r0, 96
    // Load registers for chan. 1 - 4 from bank 0
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

    QBNE    chan5to8, SAMPLE_COUNTER, R

    // Comb stages
    combs 0  // 25 cycles

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

    QBNE    chan1to4, SAMPLE_COUNTER, R
    LDI     SAMPLE_COUNTER, 0  // Reset counter to 0 this time

    // Comb stages...
    combs 8  // 25 cycles

    // Let the host know data is ready
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16

    // Go back to computing channels 1 - 4
    QBA     chan1to4
