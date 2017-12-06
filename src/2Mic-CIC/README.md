# 8 Microphones on PRU1

Here we try to implement 8 independent channels for 8 mics, each with their own CIC filter. The 8 microphones are connected to 4 data lines connected to PRU1. 

2 microphones are multiplexed on each data line using their SELECT line and an appropriate resistor. This means that one of the mics will output its data on the rising edge of the mic CLK, while the other one will output its data on the falling edge.

## Pins setup

**BBB Outputs**
* CLK : P9.14 -> all mics, PRU1 and possibly PRU0 (if we use it to do something that needs sync)
* VDD (3.3v) : P9.03 or P9.04 -> all mics
* DGND : P9.01 or P9.02 -> all mics

**PRU1 inputs**
* CLK : P8.30 <- from P9.14
* DAT1 : P8.28
* DAT2 : P8.27
* DAT3 : P8.29
* DAT4 : P8.40