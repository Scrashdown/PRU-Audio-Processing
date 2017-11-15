# Implementation of a 1-mic CIC Filter

This example tries to implement a 1-mic CIC filter.

PRU0 is used to produce the CLK signal for the microphone and nothing else.

PRU1 does the processing.

**Ideally, the CLK signal generation should be done using one of the DMTIMERs on the host side.**
