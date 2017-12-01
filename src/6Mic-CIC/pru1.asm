/**
 * Code for the CIC Filter on PRU0 with 6 channels.
 * 
 * Instruction set :
 * http://processors.Wiki.ti.com/index.php/PRU_Assembly_Instructions
 * 
 * */


#include "prudefs.hasm"

// ### Register aliases

// Local channel data, must go in scratchpads to be preserved
#define INT0_CHAN1 r0
#define INT0_CHAN2 r1
#define INT1_CHAN1 r2
#define INT1_CHAN2 r3
#define INT2_CHAN1 r4
#define INT2_CHAN2 r5
#define INT3_CHAN1 r6
#define INT3_CHAN2 r7

#define LAST_INT_CHAN1 r8
#define LAST_INT_CHAN2 r9

#define COMB0_CHAN1 r10
#define COMB0_CHAN2 r11
#define COMB1_CHAN1 r12
#define COMB1_CHAN2 r13
#define COMB2_CHAN1 r14
#define COMB2_CHAN2 r15

#define LAST_COMB0_CHAN1 r16
#define LAST_COMB0_CHAN2 r17
#define LAST_COMB1_CHAN1 r18
#define LAST_COMB1_CHAN2 r19
#define LAST_COMB2_CHAN1 r20
#define LAST_COMB2_CHAN2 r21

// Channel independent data, can stay on the PRU
#define OUTPUT1 r22
#define OUTPUT2 r23
#define OUTPUT3 r24

#define IN_PINS r31
#define TMP_REG r29
#define SAMPLE_COUNTER r28
#define LOCAL_MEM r27

// Input pins offsets, TODO: update for 3 channels
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
#define PRU0_REGS 14

// Defined in the PRU ref. guide
#define LOCAL_MEM_ADDR 0x0

#define PRU1_ARM_INTERRUPT 20

// DEBUG (assumes P8.45)
#define SET_LED SET r30, r30, 0
#define CLR_LED CLR r30, r30, 0

// Macros for integrator and comb stages
.macro integrators
    
.endm

.macro combs
.mparam memoffset
    
.endm

.origin 0
.entrypoint start

start:
    CLR_LED

    // ### Enable XIN/XOUT shift functionality ###
    LBCO    TMP_REG, C4, 0x34, 4
    SET     TMP_REG, TMP_REG, 1
    SBCO    TMP_REG, C4, 0x34, 4

    // ### Setup start configuration ###
    // Set all register values to zero, except r30 and r31, for all banks
    // Make sure bank 0 is also set to 0
    ZERO    0, 120
    XOUT    BANK0, r0, 120
    XOUT    BANK1, r0, 120
    XOUT    BANK2, r0, 120

    // Load local mem address in a register
    LDI     LOCAL_MEM, LOCAL_MEM_ADDR

    // ##### CHANNELS 1 - 4 #####
chan1to4:
    // Swap registers

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
