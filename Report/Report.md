# Project report stub

## Introduction / Motivation

The goal of this project is to implement audio processing on the BeagleBone Black's PRU microprocessor along with a C API built around it to make it easily usable as simple C library. The input signals come from 6 microphones connected to the board, each of which with a 1-bit wide signal (Pulse Density Modulation, more on that later).

The audio processing code currently handles 6 channels from 6 microphones at a fixed output sample rate. For now, the API is very simple. It allows the user to read the processed data from the PRU to a user-supplied buffer, specify the quantity of data needed and the number of channels to extract from it. It also allows the user to pause and resume the recording of data on the API side, to prevent overflows in case the user wants to momentarily stop reading data.

Running the core audio processing code on the PRU instead of the main ARM CPU allows for lower latency due to the PRU's predictable timing and it not being subject to OS scheduling like a typical Linux process running on the ARM CPU would be.

### PRU

### CIC Filter

## Documentation

### API

### Back-end

### Core processing code

## Challenges faced

### Lack of documentation

### Two drivers

### Limited number of registers

### Using threads

## Possible improvements and additional features

### Use both PRUs

### Use of a lookup table (possible with CIC filter which is IIR ?)

### Better and more modular interface (not just drop channels for example)

### Introduce further filtering on the host side