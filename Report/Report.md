# Project report stub

## Introduction / Motivation

The goal of this project is to implement audio processing using one of the BeagleBone Black's PRU microprocessors along with a C API built around it to make it easily usable as simple C library. The input signals come from 6 Knowles SPM1437HM4H-B microphones connected to the board, each of which with a 1-bit wide signal (Pulse Density Modulation, more on that later).

The audio processing code currently handles 6 channels from 6 microphones at a fixed output sample rate. For now, the API is very simple. It allows the user to read the processed data from the PRU to a user-supplied buffer, specify the quantity of data needed and the number of channels to extract from it. It also allows the user to pause and resume the recording of data on the API side, to prevent overflows in case the user wants to momentarily stop reading data.

Running the core audio processing code on the PRU instead of the main ARM CPU allows for lower latency due to the PRU's predictable timing and it not being subject to OS scheduling like a typical Linux process running on the ARM CPU would be.

## PRU / PRUSS

The PRUSS is a module of the ARM CPU used on the BeagleBone Black. It stands for PRU SubSystem, where PRU stands for Programmable Real-time Unit. The PRUSS contains 2 PRUs which are essentially very small and simple 32-bit microprocessors running at 200 MHz and using a custom instruction set. Each PRU has a constant 200 MHz clock rate, 8 kB of instruction memory, 8 kB of data memory, along with 12 kB of data memory shared between the 2 PRUs. They can be programmed either in assembly using the `pasm` assembler or in C using the `clpru` and `lnkpru` tools.

The PRUs are designed to be as time-deterministic as possible. That is, pretty much all instructions will execute in a constant number of cycles (usually 1, therefore in  5 ns at the 200 MHz clock rate) except for the memory instructions which may vary in execution time.

The PRUSS also contains an interrupt controller which allows the PRU to send receive interrupts to and from the ARM CPU. It can be configured either from the PRUs themselves by changing the values of the configuration registers, or from the ARM CPU using the API provided by the PRUSSDRV driver (more information on that below).

Using the PRUSS requires a driver. Currently, there are 2 choices available : `prussdrv` (often referred to as `UIO`) and the newer `pru_rproc`. `prussdrv` provides a lower level interface than `pru_rproc`. `pru_rproc` provides a C library for message passing between the PRU and the ARM CPU which makes programming simpler than with `prussdrv`. However, the current lack of examples online for using `pru_rproc`, along with performance issues encountered using it for this project, made us choose `prussdrv` instead.

That said, it looks like `prussdrv` is currently being phased out of support by Texas Instruments in favor of `pru_rproc`. It may be feasible in the future to convert the code to use `pru_rproc`. However, as we are going to see further in the report, the timing requirements in the PRU processing code are very tight, even using assembly. Whether it would be possible to meet them using C and `pru_rproc` has yet to be investigated.

## Audio Processing

As mentioned earlier, we are using microphones with a 1-bit wide output at a very high sample rate (> 1 MHz). The signal these microphones input is a PDM (Pulse Density Modulation) signal which is of an unusual type and needs to be converted to a lower-rate PCM (Pulse-Code Modulation) signal, which is much more commonly used for storing audio data.

![Illustration of a PCM](https://upload.wikimedia.org/wikipedia/commons/b/bf/Pcm.svg)

![Illustration of a PDM](https://upload.wikimedia.org/wikipedia/commons/2/20/Pulse-density_modulation_1_period.gif)

In a PCM signal, each value represents its amplitude on a fixed scale at a fixed time. However, in a PDM signal, its amplitude at a given time is represented by the density of 1's relative to 0's at the said time. Converting a PDM signal to a PCM signal therefore requires using some kind of a moving-average filter.

### CIC Filter

Because we wanted to run this filter on the PRU with rather tight timing constraints, we chose to implement a CIC filter. CIC stands for Cascaded Integrator-Comb filter. It is essentially an efficient implementation of a moving-average filter which uses only additions and subtractions. Although the PRU is capable of performing unsigned integer multiplications (required by other types of filters) by using its Multiply and Accumulate Unit, they take several cycles more than the one-cycle instructions used for regular additions and subtractions. Since our goal is to handle several channels at once with very tight timing constraints, computational savings really matter.

A CIC filter also has a drawback however. Its frequency response is far from the ideal flat response we would like to have. To get a sharper response, it is necessary to append another filter to it, commonly called a compensation filter. That said, since our CIC filter's output is at a lower rate (64 kHz for now) than its input (~ 1.028 MHz), applying this filter after the CIC one will require much less computational resources than applying it on the raw, very high rate input signal from the microphones.

**TODO: include an image of the frequency response**

Now let's dive into more detail about the CIC filter. The filter has 3 parameters : N, M, and R. It is made of N cascaded integrator stages, followed by a decimator of rate R, and then N cascaded comb stages, where M is the delay of the samples in the comb stages. It takes a PDM signal as input and outputs a PCM signal. If the input sample rate is `f_s`, the output sample rate will be `f_s / R`.

**TODO: include an image of a CIC decimation filter**

## Documentation

### Getting Started

First of all, make sure you have the required hardware: the BeagleBone Black, an SD card, and the Octopus Board.

#### Get UIO to work and free the GPIO pins for the PRU (*in progress*)

*Note: this was tested on this kernel:* `Linux beaglebone 4.4.91-ti-r133 #1 SMP Tue Oct 10 05:18:08 UTC 2017 armv7l GNU/Linux`

In order to run the filter, we need to be able to use the input and output pins from the PRUs. Some of them can be multiplexed to the PRUs. However, by default, some pins cannot be reassigned to something else. To correct this, we need to load a [cape](https://elinux.org/Capemgr). To do so, open the `/boot/uEnv.txt` file on the board (backup it first!) and do the following modifications :

Add the following line :

    cape_enable=bone_capemgr.enable_partno=cape-universala

And comment the following line :

    enable_uboot_cape_universal=1

You should now be able to multiplex a pin to the PRUs using the `config-pin` command (see these [instructions](Documentation/pins.md) for more details). We still have to enable the UIO driver, which is disabled in favor of remoteproc on this kernel. To do so, still in the `/boot/uEnv.txt` file, comment that line :

    uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC-4-4-TI-00A0.dtbo

And uncomment this one :

    uboot_overlay_pru=/lib/firmware/AM335X-PRU-UIO-00A0.dtbo

Then :

    $ cd /opt/source/dtb-4.4-ti
    $ sudo nano src/arm/am335x-boneblack.dts

Comment that line :

    #include "am33xx-pruss-rproc.dtsi"

And uncomment this one :

    #include "am33xx-pruss-uio.dtsi"

Then close nano and run the following commands :

    $ make
    $ sudo make install

This is the final step. Run `sudo nano /etc/modprobe.d/pruss-blacklist.conf` and add the following lines :

    blacklist pruss
    blacklist pruss_intc
    blacklist pru-rproc

Now reboot the board, and you should be able to run commands such as `config-pin -q P8.45` without trouble.


#### Install PRUSS Driver Library (`prussdrv`) and PRU Assembler (`pasm`)

In order to install the PRUSS driver on the host side, first clone this [repo](https://github.com/beagleboard/am335x_pru_package). Then `cd` into the cloned repository and run the following commands :

    $ make
    $ sudo make install
    $ sudo ldconfig

If everything went well, the `prussdrv` library and the `pasm` assembler should be installed on your board and ready to be used.

### The microphones

### Core processing code

### Back-end

### API

The C interface is written in the `interface.h` and `interface.c` files. It is currently very simple and provides the following functions :

```C
/**
* @brief Initialize PRU processing. Must be called before any other function of this file.
* 
* @return pcm_t* A pointer to a new pcm object in case of success, NULL otherwise.
*/
pcm_t * pru_processing_init(void);

/**
* @brief Stop processing and free/close all resources.
* 
* @param pcm The pcm object containing the resources.
*/
void pru_processing_close(pcm_t * pcm);
    
/**
 * @brief Read a given number of blocks of given size and output them to the user provided buffer.
 * 
 * @param src The source pcm from which to read.
 * @param dst The buffer to which we want to write data.
 * @param nsamples The number of samples to read from each channel.
 * @param nchan The number of channels to read.
 * @return int The number of samples effectively written.
 */
int pcm_read(pcm_t * src, void * dst, size_t nsamples, size_t nchan);

/**
* @brief Enable recording of the audio to the ringbuffer.
* 
* Once this function is called, the interface will start copying
* from the buffer the PRU writes to, to the main ringbuffer of the interface. In order to avoid
* a ringbuffer overflow, the user must therefore start reading using pcm_read quickly after this
* function had been called.
* 
*/
void enable_recording(void);

/**
* @brief Disable recording of the audio to the ringbuffer.
* 
* Once this function is called, the interface will stop copying data to the main ringbuffer.
* This means only the samples remaining in the ringbuffer after this function was called can be read.
* 
*/
void disable_recording(void);
```

## Challenges faced

### Lack of documentation and the existence of 2, very different drivers

The biggest challenge faced in this project is probably the lack of clear and organized documentation about how to run code on the PRU from the Linux host, how to configure the operating system so that the BeagleBone's pins can be multiplexed to the PRU, how to choose which driver to use, and finally how to configure the BeagleBone for it to work. Most of the documentation and examples are scarce, sometimes outdated and scattered across multiple websites which forced us to do a lot of trial and errors on things such has how to enable drivers or the right interrupts between the host and the PRU.

Apart from the fact that embedded systems is an inherently tough subject that is by far not as popular as more high level programming is (especially about the PRU, which seems to be a piece of hardware very few people use or know about), I think the scarcity of the documentation is probably the greatest factor that makes the learning curve for this project rather steep.

### Limited number of registers and very tight timings on the PRU

On a more technical point of view, processing six channels simultaneously on one PRU is feasible, but challenging in terms of resource management. In our current implementation of the 6-channels CIC filter on the PRU, all operations required for processing one sample from each channel must execute in less than TODO: cycles. All of the PRU's registers are used, and the majority of the banks' registers are used as well.

### Using threads

## Possible improvements and additional features

### Use both PRUs

Currently, we use only one PRU (PRU1) to handle the audio processing with the CIC filter. The design choice was made to make the implementation simpler. However, this also limited us to being only able to process 6 channels at a time instead of the initial goal of 8.

It could for example be possible to implement a CIC on both PRUs which would allow us to handle more than 6 channels. Another idea would be to keep the CIC filter on one PRU, but move the compensation filter which is currently implemented on the host ARM CPU to the other PRU, offloading the ARM CPU even further and also reducing the latency.

### Use of a lookup table (possible with CIC filter which is IIR ?)

### Better and more modular interface (not just drop channels for example)

### Introduce further filtering on the host side