# 6 Microphones on PRU1 with a C interface

6 microphones with 6 channels output. Each microphone has his own separate CIC filter. The microphones are connected on 3 separate datalines.

2 microphones are multiplexed on each data line using their SELECT line and an appropriate resistor. This means that one of the mics will output its data on the rising edge of the mic CLK, while the other one will output its data on the falling edge.

A very simple C interface is also provided and is described in the main README.md file.

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