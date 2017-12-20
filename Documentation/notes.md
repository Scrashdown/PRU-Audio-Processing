# Notes for the project

**XCHG** instruction behaves like **XIN**, so we unfortunately need some extra temporary storage if we use scratch pads.

It might be possible to modify / create macros in .asm files using `pasm -D` option.

## Detecting missed buffer halves

This is tricky. For now, processing for both halves is done in a single thread. To ensure the doesn't desynchronized from the PRU and starts reading the wrong half of the buffer each time, if it missed a PRU interrupt, we wait for both events one after the other. This is a simple solution that works but doesn't allow us to detect if a buffer half has been skipped.

In order to detect this, we would have to use 2 threads. One for each buffer half, where each thread would wait for the corresponding PRU interrupt. There would be a flag for the expected next interrupt. Just after receiving an interrupt, each thread would check if the received interrupt was the right one (output a warning if not), then do the processing, finally set the flag to the other thread's interrupt and loop back.