/**
 * @brief Simple 1 channel CIC filter.
 * 
 * @author Loïc Droz <lk.droz@gmail.com>
 * 
 */

#include "prudefs.hasm"

// TODO: Figure out how interrupts work
#define PRU1_ARM_INTERRUPT 20

#define R 16

// Input pins offsets
#define CLK_OFFSET 11
#define DATA_OFFSET 10

// Register aliases
#define IN_PINS r31
#define SAMPLE_COUNTER r5
#define WAIT_COUNTER r6
#define TMP_REG r7
#define BYTE_COUNTER r8

#define HOST_MEM r20
// Host mem size is multiple of 8, this is ensured on the host side
#define HOST_MEM_SIZE r21
#define LOCAL_MEM r22
// Defined in page 19 of the AM335x PRU-ICSS Reference guide
#define LOCAL_MEM_ADDR 0x0000

#define INT0 r0
#define INT1 r1
#define INT2 r2
#define INT3 r3
#define LAST_INT r4

#define COMB0 r10
#define COMB1 r11
#define COMB2 r12
#define LAST_COMB0 r14
#define LAST_COMB1 r15
#define LAST_COMB2 r16

// DEBUG (assumes P8.45)
#define SET_LED SET r30, r30, 0
#define CLR_LED CLR r30, r30, 0
#define TOGGLE_LED XOR r30, r30, 1

.origin 0
.entrypoint start

start:
    // ### Memory management ###
    // Enable OCP master ports in SYSCFG register
    // It is okay to use the r0 register here (which we use later too) because it merely serves as a mean to temporary hold the value of C4 + 4, the OCP masters are enabled by writing the correct data to C4
    LBCO    r0, C4, 4, 4
    CLR     r0, r0, 4
    SBCO    r0, C4, 4, 4
    // Load the local memory address in a register
    MOV     LOCAL_MEM, LOCAL_MEM_ADDR
    // From local memory, grab the address of the host memory (passed by the host before this program started)
    LBBO    HOST_MEM, LOCAL_MEM, 0, 4
    // Likewise, grab the host memory length
    LBBO    HOST_MEM_SIZE, LOCAL_MEM, 4, 4

    // ### Set up start configuration ###
    // Setup counters to 0 at first
    LDI     SAMPLE_COUNTER, 0
    LDI     BYTE_COUNTER, 0
    // Set all integrator and comb registers to 0 at first
    LDI     INT0, 0
    LDI     INT1, 0
    LDI     INT2, 0
    LDI     INT3, 0
    LDI     COMB0, 0
    LDI     COMB1, 0
    LDI     COMB2, 0
    LDI     LAST_INT, 0
    LDI     LAST_COMB0, 0
    LDI     LAST_COMB1, 0
    LDI     LAST_COMB2, 0


    // ### Signal processing ###
wait_edge:  // XXX: TESTED and WORKS
    // First wait for CLK = 0
    WBC     IN_PINS, CLK_OFFSET
    // Then wait for CLK = 1
    WBS     IN_PINS, CLK_OFFSET

    // Wait for t_dv time, since it can be at most 125ns, we have to wait for 25 cycles
    LDI     WAIT_COUNTER, 12 // Because 25 = 1 + 12*2 and the loop takes 2 one-cycle ops
wait_signal:
    SUB     WAIT_COUNTER, WAIT_COUNTER, 1
    QBNE    wait_signal, WAIT_COUNTER, 0

    // Retrieve data from DATA pin (only one bit!)
    LSR     TMP_REG, IN_PINS, DATA_OFFSET
    AND     TMP_REG, TMP_REG, 1
    // Do the integrator operations
    ADD     SAMPLE_COUNTER, SAMPLE_COUNTER, 1
    ADD     INT0, INT0, TMP_REG
    ADD     INT1, INT1, INT0
    ADD     INT2, INT2, INT1
    ADD     INT3, INT3, INT2

    // Branch for oversampling
    QBNE    wait_edge, SAMPLE_COUNTER, R

    // Reset sample counter once we reach R
    LDI     SAMPLE_COUNTER, 0

    // 4 stage comb filter
    SUB     COMB0, INT3, LAST_INT
    SUB     COMB1, COMB0, LAST_COMB0
    SUB     COMB2, COMB1, LAST_COMB1
    SUB     TMP_REG, COMB2, LAST_COMB2

    // Output the result to memory
    // We write one word (4 B) from TMP_REG to HOST_MEM with an offset of BYTE_COUNTER
    SBBO    TMP_REG, HOST_MEM, BYTE_COUNTER, 4
    // Increment the written bytes counter once the write operation is done
    ADD     BYTE_COUNTER, BYTE_COUNTER, 4
    // First, check if we are about to overrun the buffer, that is, if HOST_MEM_SIZE - BYTE_COUNTER < 4
    // If yes, send an interrupt to the host, and reset the byte counter/offset back to 0
    QBNE    check_half, BYTE_COUNTER, HOST_MEM_SIZE
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16  // Interrupt the host, TODO: could be done in a safer way by writing to the host memory which buffer we're in
    LDI     BYTE_COUNTER, 0  // Reset counter/offset, which will make us write to the beginning of host memory again
    QBA     continue_comb

check_half:
    // If we have filled more than half of the buffer on the host side, send an interrupt, use TMP_REG to store the value of the host buffer divided by 2, because the host side memory length is a multiple of 8, so half of it will be a multiple of 4
    LSR     TMP_REG, HOST_MEM_SIZE, 1
    QBNE    continue_comb, TMP_REG, BYTE_COUNTER
    // Interrupt the host to tell him we wrote to half of the buffer
    MOV     r31.b0, PRU1_ARM_INTERRUPT + 16

continue_comb:
    // Update LAST_INT value and LAST_COMBs
    // TODO: check this is correct, and this could perhaps be done in fewer instructions
    MOV     LAST_INT, INT3
    MOV     LAST_COMB0, COMB0
    MOV     LAST_COMB1, COMB1
    MOV     LAST_COMB2, COMB2

    // Branch back to wait
    QBA     wait_edge