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

// DEBUG (assumes P8.11)
#define SET_LED SET r30, r30, 15
#define CLR_LED CLR r30, r30, 15

.origin 0
.entrypoint start

start:
    CLR_LED

    // Memory address
    LDI     r1, 0

    LDI     r1.t0, 1
    
    SBBO    r0, r1, 0, 4

end:
    HALT