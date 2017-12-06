/**
 * Code for the CIC Filter on PRU0 with 6 channels.
 * 
 * Instruction set :
 * http://processors.Wiki.ti.com/index.php/PRU_Assembly_Instructions
 * 
 * Current timings:
 * 
 * Rising edge data : 53 cycles
 * Falling edge data : 58 cycles
 * 
 * 
 * 
 * */


#include "prudefs.hasm"

// ### Register aliases

// ## Local channel data, must go in scratchpads to be preserved
// # First channel data
#define INT0_CHAN1 r1
#define INT1_CHAN1 r2
#define INT2_CHAN1 r3
#define INT3_CHAN1 r4

#define LAST_INT_CHAN1 r5

#define COMB0_CHAN1 r6
#define COMB1_CHAN1 r7
#define COMB2_CHAN1 r8

#define LAST_COMB0_CHAN1 r9
#define LAST_COMB1_CHAN1 r10
#define LAST_COMB2_CHAN1 r11

// # Second channel data
#define INT0_CHAN2 r12
#define INT1_CHAN2 r13
#define INT2_CHAN2 r14
#define INT3_CHAN2 r15

#define LAST_INT_CHAN2 r16

#define COMB0_CHAN2 r17
#define COMB1_CHAN2 r18
#define COMB2_CHAN2 r19

#define LAST_COMB0_CHAN2 r20
#define LAST_COMB1_CHAN2 r21
#define LAST_COMB2_CHAN2 r22

// ## Channel independent data, can stay on the PRU
#define OUTPUT1 r23
#define OUTPUT2 r24

#define IN_PINS r31
#define TMP_REG r29
#define SAMPLE_COUNTER r28
#define LOCAL_MEM r27

// Input pins offsets, TODO: update for 3 channels
#define CLK_OFFSET 11
#define DAT_OFFSET1 10
#define DAT_OFFSET2 8

// Decimation rate
#define R 16

// Defined in the PRU ref. guide
#define LOCAL_MEM_ADDR 0x0

#define PRU1_ARM_INTERRUPT 20

// DEBUG (assumes P8.45)
#define SET_LED SET r30, r30, 0
#define CLR_LED CLR r30, r30, 0
#define TOGGLE_LED XOR r30, r30, 1


.macro int_comb_chan12  // 29 cycles
.mparam jmp_addr
    // Retrieve data for the 2 first channels and do the integrator stages
        LSR     TMP_REG, IN_PINS, DAT_OFFSET1
        AND     TMP_REG, TMP_REG, 1
        ADD     INT0_CHAN1, INT0_CHAN1, TMP_REG

        LSR     TMP_REG, IN_PINS, DAT_OFFSET2
        AND     TMP_REG, TMP_REG, 1
        ADD     INT0_CHAN2, INT0_CHAN2, TMP_REG

        ADD     INT1_CHAN1, INT1_CHAN1, INT0_CHAN1
        ADD     INT1_CHAN2, INT1_CHAN2, INT0_CHAN2

        ADD     INT2_CHAN1, INT2_CHAN1, INT1_CHAN1
        ADD     INT2_CHAN2, INT2_CHAN2, INT1_CHAN2

        ADD     INT3_CHAN1, INT3_CHAN1, INT2_CHAN1
        ADD     INT3_CHAN2, INT3_CHAN2, INT2_CHAN2

        // Work on channel 3
        QBNE    jmp_addr, SAMPLE_COUNTER, R

        SUB     COMB0_CHAN1, INT3_CHAN1, LAST_INT_CHAN1
        SUB     COMB0_CHAN2, INT3_CHAN2, LAST_INT_CHAN2

        SUB     COMB1_CHAN1, COMB0_CHAN1, LAST_COMB0_CHAN1
        SUB     COMB1_CHAN2, COMB0_CHAN2, LAST_COMB0_CHAN2

        SUB     COMB2_CHAN1, COMB1_CHAN1, LAST_COMB1_CHAN1
        SUB     COMB2_CHAN2, COMB1_CHAN2, LAST_COMB1_CHAN2

        // Store the output in the PRU1 "private" output registers
        SUB     OUTPUT1, COMB2_CHAN1, LAST_COMB2_CHAN1
        SUB     OUTPUT2, COMB2_CHAN2, LAST_COMB2_CHAN2

        MOV     LAST_INT_CHAN1, INT3_CHAN1
        MOV     LAST_INT_CHAN2, INT3_CHAN2
        MOV     LAST_COMB0_CHAN1, COMB0_CHAN1
        MOV     LAST_COMB0_CHAN2, COMB0_CHAN2
        MOV     LAST_COMB1_CHAN1, COMB1_CHAN1
        MOV     LAST_COMB1_CHAN2, COMB1_CHAN2
        MOV     LAST_COMB2_CHAN1, COMB2_CHAN1
        MOV     LAST_COMB2_CHAN2, COMB2_CHAN2
.endm


.origin 0
.entrypoint start

start:
    CLR_LED

    // ### Setup start configuration ###
    // Set all register values to zero, except r30 and r31, for all banks
    ZERO    0, 120

    // Load local mem address in a register
    LDI     LOCAL_MEM, LOCAL_MEM_ADDR


    // ##### CHANNELS 1 - 2 #####
wait_rising_edge:

    // Wait for rising edge
    WBC     IN_PINS, CLK_OFFSET
    WBS     IN_PINS, CLK_OFFSET

    // Update sample counter for decimation
    ADD     SAMPLE_COUNTER, SAMPLE_COUNTER, 1

    // Wait for t_dv time, since it can be at most 125ns, we have to wait for 24 + 5 cycles
    LDI     TMP_REG, 10
wait_data:
    SUB     TMP_REG, TMP_REG, 1
    QBNE    wait_data, TMP_REG, 0

chan12:
    // Integrator and comb stages
    int_comb_chan12 wait_rising_edge  // 29 cycles

    // If we reach this point, it means we reached R, so reset the counter
    LDI     SAMPLE_COUNTER, 0

    // Move the results of the filter to PRU memory
    SBBO    OUTPUT1, LOCAL_MEM, 0, 8

    // Let the host know data is ready
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16

    // Go back to computing channels 1 - 4
    QBA     wait_rising_edge
