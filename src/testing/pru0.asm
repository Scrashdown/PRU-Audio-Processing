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

// TODO: Figure out how interrupts work
#define PRU1_ARM_INTERRUPT 20

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
    CLR_LED

    MOV     r0.w0, 0xFFFF
    MOV     r0.w1, 0xFFFF
    ADD     r0, r0, 1

    // Should overflow
    QBNE    end, r0, 0
    SET_LED

end:
    HALT