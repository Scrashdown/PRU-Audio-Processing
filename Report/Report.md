# Project report stub

## Introduction / Motivation

The goal of this project is to implement audio processing using one of the BeagleBone Black's PRU microprocessors along with a C API built around it to make it easily usable as simple C library. The input signals come from 6 Knowles SPM1437HM4H-B microphones connected to the board, each of which with a 1-bit wide signal (Pulse Density Modulation, more on that later).

The audio processing code currently handles 6 channels from 6 microphones at a fixed output sample rate, using one of the 2 PRUs present on the board. For now, the API is very simple. It allows the user to read the processed data from the PRU to a user-supplied buffer, specify the quantity of data needed and the number of channels to extract from it. It also allows the user to pause and resume the recording of data on the API side, to prevent overflows in case the user wants to momentarily stop reading data.

Running the core audio processing code on the PRU instead of the main ARM CPU allows for lower latency due to the PRU's predictable timing and it not being subject to OS scheduling like a typical Linux process running on the ARM CPU would be. Offloading the ARM CPU from such an intensive task also prevents our library from having a significant impact on the global performance of the host when it is used.

## PRU / PRUSS

The PRUSS is a module of the ARM CPU used on the BeagleBone Black. It stands for PRU SubSystem, where PRU stands for Programmable Real-time Unit. The PRUSS contains 2 PRUs which are essentially very small and simple 32-bit microprocessors running at 200 MHz and using a custom instruction set. Each PRU has a constant 200 MHz clock rate, 8 kB of instruction memory, 8 kB of data memory, along with 12 kB of data memory shared between the 2 PRUs. They can be programmed either in assembly using the `pasm` assembler or in C using the `clpru` and `lnkpru` tools.

Each PRU has 32, 32-bit registers, where R30 is used for interacting with the PRU's output pins and R31 is used for reading inputs and triggering interrupts by writing to it. Both PRUs also share 3 registers banks, also called scratchpads, which are banks containing 30 additional registers. Registers can be transferred between the banks and each PRU in one cycle by using specialized assembly instructions. Furthermore, one PRU can also access the register of the other PRU.

The PRUs are designed to be as time-deterministic as possible. That is, pretty much all instructions will execute in a constant number of cycles (usually 1, therefore in  5 ns at the 200 MHz clock rate) except for the memory instructions which may vary in execution time.

The PRUSS also contains an interrupt controller which allows the PRU to send receive interrupts to and from the ARM CPU. It can be configured either from the PRUs themselves by changing the values of the configuration registers, or from the ARM CPU using the API provided by the PRUSSDRV driver (more information on that below).

Using the PRUSS requires a driver. Currently, there are 2 choices available : `uio_pruss` (along with the `prussdrv` library) and the newer `pru_rproc`. `oui_pruss` provides a lower level interface than `pru_rproc`. `pru_rproc` provides a C library for message passing between the PRU and the ARM CPU which makes programming simpler than with `uio_pruss`. However, the current lack of examples online for using `pru_rproc`, along with performance issues encountered using it for this project, made us choose `uio_pruss` instead.

That said, it looks like `uio_pruss` is currently being phased out of support by Texas Instruments in favor of `pru_rproc`. It may be feasible in the future to convert the code to use `pru_rproc`. However, as we are going to see further in the report, the timing requirements in the PRU processing code are very tight, even using assembly. Whether it would be possible to meet them using C and `pru_rproc` has yet to be investigated.

## Audio Processing

As mentioned earlier, we are using microphones with a 1-bit wide output at a very high sample rate (> 1 MHz). The signal these microphones input is a PDM (Pulse Density Modulation) signal which is of an unusual type and needs to be converted to a lower-rate PCM (Pulse-Code Modulation) signal, which is much more commonly used for storing audio data.

![Illustration of a PCM](https://upload.wikimedia.org/wikipedia/commons/b/bf/Pcm.svg)

![Illustration of a PDM](https://upload.wikimedia.org/wikipedia/commons/2/20/Pulse-density_modulation_1_period.gif)

In a PCM signal, each value represents its amplitude on a fixed scale at a fixed time. However, in a PDM signal, its amplitude at a given time is represented by the density of 1's relative to 0's at the said time. Converting a PDM signal to a PCM signal therefore requires using some kind of a moving-average filter.

### CIC Filter

Because we wanted to run this filter on the PRU with rather tight timing constraints, we chose to implement a CIC filter. CIC stands for Cascaded Integrator-Comb filter. There are two general types of CIC filters : the decimation filters, which is the type we're using in this project, and the interpolation filters. From now on we will call CIC decimator filters simply CIC filters.

It is essentially an efficient implementation of a moving-average filter which uses only additions and subtractions and also has a finite impulse response (FIR). Although the PRU is capable of performing unsigned integer multiplications (required by other types of filters) by using its Multiply and Accumulate Unit, they take several cycles more to execute than the one-cycle instructions used for regular additions and subtractions. Since our goal is to handle several channels at once with very tight timing constraints, computational savings really matter.

A CIC filter also has a drawback however. Its frequency response is far from the ideal flat response with a sharp cutoff we would like to have. To get a sharper cutoff, it is necessary to append another filter to it, commonly called a compensation filter. That said, since our CIC filter's output is at a lower rate (64 kHz for now) than its input (~ 1.028 MHz), applying this filter after the CIC one will require much less computational resources than applying it on the raw, very high rate input signal from the microphones.

![Example power response of the filter](Pictures/CIC_power_resp.png)

Now let's dive into more detail about the CIC filter. The filter has 3 parameters : N, M, and R. It is made of N cascaded integrator stages, followed by a decimator of rate R, and then N cascaded comb stages, where M is the delay of the samples in the comb stages. It takes a PDM signal as input and outputs a PCM signal. If the input sample rate is `f_s`, the output sample rate will be `f_s / R`. A CIC interpolation filter is very similar, except the comb stages come first, followed by an interpolator of rate R, and finally the integrators.

![Structure of a CIC decimation filter](Pictures/CIC_structure.png)

The filter's resource usage depends on its parameters, the platform on which it is implemented and how it is implemented. More detailed explanation will follow in the implementation section of this report. However, by considering only the theoretical structure of the filter, we can already deduce some general rules :

* Memory usage is more or less proportional to N and M : The filter has N integrator stages and N comb stages of which we need to store the values, therefore memory usage will increase with N. Also, since M is the delay of the samples in the comb stages, for each comb stage it is necessary to store the previous samples up to M, therefore memory usage will also increase with M.
* Computational resource usage is inversely correlated to R : Since the comb stages are preceded by a decimator of rate R, the comb stages need to be updated R times less often than the integrator stages. Therefore, as R increases, less computational power is required by the comb stages. However, the reduction cannot be arbitrarily high, because the integrator stages always need to be updated at the very high input sample rate, independently of R. The processing power needed for these stages, and any other overhead added by the implementation, is therefore a lower-bound of the the total processing power required to run the filter.

Finally, some important things to know about CIC decimation filters is that overflows will occur in the integrator stages, however, the output of the filter will still be correct if each stage follows modular arithmetic rules (PRU registers do, since they can overflow and loop back to zero) and if each stage has a bit width large enough that it can support the maximum value expected at the end of the filter. To check the second condition, a formula exists to find out about the required bit width of the stages for the filter to be correct :

$$B_{out} = \lceil Nlog_2RM + B_{in} \rceil$$

Where $B_in$ is the bit width of the filter's input (in our case 1 bit), M, N and R are the filter's parameters, and $B_{out}$ is the required bit width for all stages of the filter.

## Documentation

### Getting Started

First of all, make sure you have the required hardware: the BeagleBone Black, an SD card, and the Octopus Board. Flash the board with the latest "IoT" Debian image following these [instructions](https://beagleboard.org/getting-started).

**TODO: include image of the hardware, of possible**

#### Configure uio_pruss and free the GPIO pins for the PRU (*in progress*)

*Note: this was tested on this kernel:* `Linux beaglebone 4.4.91-ti-r133 #1 SMP Tue Oct 10 05:18:08 UTC 2017 armv7l GNU/Linux`

In order to run the filter, we need to be able to use the input and output pins from the PRUs to be able to read data from the microphones. Some of them can be multiplexed to the PRUs. However, by default, some pins cannot be reassigned to something else. To correct this, we need to load a [cape](https://elinux.org/Capemgr). To do so, open the `/boot/uEnv.txt` file on the board (backup it first!) and do the following modifications :

Add the following line :

    cape_enable=bone_capemgr.enable_partno=cape-universala

And comment the following line :

    enable_uboot_cape_universal=1

You should now be able to multiplex a pin to the PRUs using the `config-pin` command (see these [instructions](Documentation/pins.md) for more details). We still have to enable the UIO driver, which is disabled in favor of `pru_rproc` on this kernel. To do so, still in the `/boot/uEnv.txt` file, comment that line :

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

#### Plug the Octopus board and write some code!

```C
/* Example code for using the library. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "interface.h"

#define OUTFILE "../output/interface.pcm"
#define NSAMPLES 64000
#define NCHANNELS 6

int main(void) {
    FILE * outfile = fopen(OUTFILE, "w");
    void * tmp_buffer = calloc(NSAMPLES, NCHANNELS * SAMPLE_SIZE_BYTES);
    pcm_t * pcm = pru_processing_init();
    
    struct timespec delay = { 1, 0 };
    enable_recording();
        nanosleep(&delay, NULL);
    disable_recording();

    pcm_read(pcm, tmp_buffer, NSAMPLES, NCHANNELS);
    pru_processing_close(pcm);

    fwrite(tmp_buffer, NCHANNELS * SAMPLE_SIZE_BYTES, NSAMPLES, outfile);

    fclose(outfile);
    free(tmp_buffer);
    return 0;
}

```

### Microphones and wiring diagram

![Picture of the microphones we're using](Pictures/mics.png)

For this project, we are using the Knowles SPM1437HM4H-B microphones which output a PDM signal at a very high frequency (> 1 MHz), see the microphone's data sheet in the documentation for more details. They have 6 pins :

* 2 x **GROUND** (power) : Ground
* **Vdd** (power) : Vdd
* **CLOCK** (input) : The clock input, must be at a frequency > 1 MHz to wake up the microphone. Dictates the microphone's sample rate, `f_s = f_clk`.
* **DATA** (output) : The microphone's PDM output. Its sample rate equals that of the CLOCK signal. The DATA signal is ready shortly after the rising or falling edge of the CLK. This delay is variable, but is between 18 ns and 125 ns. It is designated in the documentation as `t_dv`.
* **SELECT** (input) : Selects whether data is ready after rising or falling edge of CLOCK, (VDD => rising, GND => falling).

The BeagleBone Black already has pins for GND and Vdd, we connect them directly to the corresponding pins on the microphones.

The CLK signal is generated using one of the BeagleBone's internal PWM which is output on one of the board's pins and is configured by a bash script to generate an appropriate CLK signal for the microphones. In our implementation, the PRU's also need to be able to poll the state of the CLK signal. In order to achieve this, the PWM (CLK) signal is also connected to some of the BeagleBone's pins, which are multiplexed to the PRU.

The DATA pin of the microphone is then connected to a pin of the board which is multiplexed to the PRU.

![Wiring of one microphone to the board](Pictures/wiring.png)

Since we are using 6 microphones, we could use 6 pins on the board for the microphone's DATA outputs, however it is possible to connect 2 microphones per board's pin. To achieve this, we set one of the SELECT lines of the microphones to 2 different values. By doing this, one of the microphones will have data ready just after the rising edge of the input clock, while the other one will have data ready just after the falling edge. In order to avoid short circuits between the 2 microphones connected to the same pin, it is necessary to place a resistor between each microphone and the pin.

Doing this has a drawback however, for each round of processing (processing all the channels) we need to wait for `t_dv` twice instead of only once with the 'simple' solution using 6 pins. This is because in this case we have to wait for a clock edge twice. According to the microphone's datasheet, `t_dv` can go up to 125 ns, which is 25 cycles of the PRU at its 200 MHz clock rate. Although not a huge advantage in performance it is still significant. However, the current Octopus board uses the 3 pins for 6 microphones, so we have to deal with this drawback.

![Timing diagram of the microphones](Pictures/timing_diagram.png)

### Overview of the whole processing chain

![](Pictures/PRU_processing_chain.png)

### Core processing code

The core audio processing code, which implements the CIC filter, is running on the PRU and handles the tasks of reading the data from the microphones in time, processing all the channels, writing the results directly into the host's memory (where a fixed size buffer has been allocated by `prussdrv`) and interrupting the host whenever data is ready to be retrieved by the host from its buffer. It is written exclusively in PRU assembly (`pru1.asm` in the project files).

For performance reasons, the PRU uses registers to store the data of each stage of the filter. Because the PRU only has 30 available registers for storing data, it needs to use registers from the scratchpad as well. It does so by exchanges some of its registers with the scratchpads using the XIN and XOUT instructions.

Since we are using the Octopus board, we have to read data at every edge of the clock. The processing is done in 3 major steps : first we read the data for the channels 1, 2 and 3, process it and output it to the host memory, then we do the same for channels 4, 5 and 6, and finally we interrupt the host to let it know data for all channels is ready. We also have to keep track of several different counters along the way, and also do the correct register exchanges to keep any data from being overwritten. The counters are needed for implementing the decimator, waiting for `t_dv` and keeping track of how many samples have been written to the host's memory, so that it can be interrupted when some data is ready.

**TODO: add timing diagram with processing**

In order to allow the host to retrieve all the samples before the PRU overwrites them with new data, we have the PRU trigger an interrupt whenever it reaches the middle of the buffer, or the end. These interrupts have different codes which allows the host to tell which half of the buffer contains fresh data. This way, the host can be sure to read one half of the buffer while the other half is being overwritten by the PRU.

![](Pictures/PRU_buffer.png)

Reading the microphones' data is achieved by reading bits of the R31 register, which is connected to the PRU's input pins, to which the microphones' DATA lines are connected. Since we connected the microphones' clock to one of the input pins of the PRU as well, reading its state is done the same way. In order to know when to read the data, the PRU polls the CLK signal until it detects an edge. It then waits for 25 cycles (`t_dv`) and finally reads the data and stores it in a register.

This register is the first integrator in the CIC filter and all subsequent integrators can be updated. Once this is done, the decimator is implemented by using a counter. The uses that counter to determine whether it is time to update the comb stages or not. Once the comb stages are updated, some of the results of the CIC filter are ready and are written to the host's memory.

### Host interface and API

The host interface is written in C in the `interface.h` and `interface.c` files. It is currently very simple and provides the following functions :

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

The host interface contains several components. The first is a special buffer in the host memory allocated by `prussdrv` which the PRU directly writes to. It contains the data computed by the PRU for all channels. The second component is a larger circular buffer which serves as the main temporary buffer of the interface.

When the `pru_processing_init()` function of the API is called, it loads and starts the PRU firmware, and starts a separate thread which handles the recording the samples from the PRU buffer to the bigger circular buffer of the interface. Everytime the PRU has finished writing one half of this buffer, it sends an interrupt to the host and starts writing to the other half. The code of the interrupt sent designates which half of the buffer was filled with fresh data.

This thread essentially waits alternatively for each interruption and then copies the corresponding buffer half to the circular buffer. Waiting for each interrupt alternatively makes sure the host cannot get desynchronized and start writing the buffer halves that are being overwritten by the PRU. However, this mechanism cannot detect whether a buffer half has been skipped. This can happen if the host is too busy with other programs which can make it miss an interrupt by the PRU. Furthermore, if an interrupt is missed, the next one will be skipped as well, since we wait for them alternatively. Therefore if an interrupt is missed, 2 buffer halves will actually be missed. We chose to use this mechanism because of its simplicity, and because it did not require the PRU to count the buffer halves it filled and output the number to the host regularly. This would however be a better solution.

The circular buffer is similar to a queue but with a fixed length. Its API provides functions for pushing and popping data from it. In the case of an overflow, the push function will only throw a warning and overwrite the oldest data in the buffer.

The `pcm_read` function provided by the API pops the number of samples required by the user from the circular buffer, then extracts only the number of channels the user asks for, and finally outputs this data to the user provided buffer.

## Challenges faced

### Lack of documentation and the existence of 2, very different drivers

The biggest challenge faced in this project is probably the lack of clear and organized documentation about how to run code on the PRU from the Linux host, how to configure the operating system so that the BeagleBone's pins can be multiplexed to the PRU, how to choose which driver to use, and finally how to configure the BeagleBone for it to work. Most of the documentation and examples are scarce, sometimes outdated and scattered across multiple websites which forced us to do a lot of trial and errors on things such has how to enable drivers or the right interrupts between the host and the PRU.

Apart from the fact that embedded systems is an inherently tough subject that is by far not as popular as more high level programming is (especially about the PRU, which seems to be a piece of hardware very few people use or know about), I think the scarcity of the documentation is probably the greatest factor that makes the learning curve for this project rather steep.

### Limited number of registers and tight timings

On a more technical point of view, processing six channels simultaneously on one PRU is feasible, but challenging in terms of resource management. In our current implementation of the 6-channels CIC filter on the PRU, all operations required for processing one sample from each channel must execute in less than TODO: cycles. All of the PRU's registers are used, and the majority of the banks' registers are used as well.

Another challenge was to design the program such that it would not rely on the host too much because of its unpredictable timings and busy nature. Below is an example of a bug we had when the host was in charge of retrieving a new sample everytime it was ready (with these parameters, 64000 times per second). The host couldn't keep up and missed many samples, resulting in this quite weird looking (and sounding) timing diagram.

![Spectrogram of a signal missing samples as a result of timing issues](Pictures/timing_bug.png)

## Possible improvements and additional features

### Use both PRUs

Currently, we use only one PRU (PRU1) to handle the audio processing with the CIC filter. The design choice was made to make the implementation simpler. However, this also limited us to being only able to process 6 channels at a time instead of the initial goal of 8.

It could for example be possible to implement a CIC on both PRUs which would allow us to handle more than 6 channels. Another idea would be to keep the CIC filter on one PRU, but move the compensation filter which is currently implemented on the host ARM CPU to the other PRU, offloading the ARM CPU even further and also reducing the latency.

### Tweak the parameters to get smaller bit width and possibly handle more channels

The parameters we are currently using for the CIC filter (R = 16, N = 4, M = 1) give a decent frequency response (at least judging by our ear) but require 17 bits per stage of the filter. Tweaking the parameters to achieve 16 bits or less would allow fitting 2 channels in one register which would dramatically reduce the usage of memory resources, both on the PRU and on the host.

### Use of a lookup table

Since the PRUs each have some data memory available (8 kB each, with an additional shared 12 kB), it might be more time-efficient to implement the PDM to PCM conversion using a precomputed look-up table stored in memory instead of implementing a CIC filter.

### Better and more modular interface

For now the interface is very limited, and depending on how many channels the user chooses to read, the whole program can also be very wasteful on resources. This is because with the current implementation, the PRU always processes the 6 channels, and the host interface's backend always records all 6 channels, even if in the end the user requests fewer channels. In the event the user wants to read fewer channels, the interface's front-end will just drop the data from the channels the user does not want, before sending the data to the user.

An improvement could be to let the user choose how many channels he intends to use at most, and then only handle this number of channels instead of the maximum possible. However, making the CIC filter's code modular might not be a feasible task given the high performance requirements, at least with the current model of our implementation. A workaround would be to write several programs, possibly one for each number of channels.

### Introduce further filtering on the host side

As mentioned before, CIC filters are very efficient filters but they lack a flat frequency response with a sharp cutoff and we need an additional compensation filter appended after them to get a better response. The current implementation of the host interface does implement such a filter yet but this is a possible and probably very useful improvement that could be made. A nice feature to have could be to make it modular such that it can accept many different types of compensation filters.