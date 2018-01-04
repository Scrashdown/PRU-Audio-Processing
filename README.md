# PRU-Audio-Processing *(in progress!)*

This is the repository of my bachelor's project. The project aims to run a microphone array on the BeagleBone Black, using the PRU Sub System to process the raw data from the microphones and offload it from the main ARM CPU. We also aim to provide an C API around the core audio processing core.

## Interface

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

## Getting Started

### Get UIO to work and free the GPIO pins for the PRU (*in progress*)

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


### Install PRUSS Driver Library (`prussdrv`) and PRU Assembler (`pasm`)

In order to install the PRUSS driver on the host side, first clone this [repo](https://github.com/beagleboard/am335x_pru_package). Then `cd` into the cloned repository and run the following commands :

    $ make
    $ sudo make install
    $ sudo ldconfig

If everything went well, the `prussdrv` library and the `pasm` assembler should be installed on your board and ready to be used.

## Useful links

Sharing internet with the BeagleBone : http://www.dangtrinh.com/2015/05/sharing-internet-with-beaglebone-black.html

## Sources

UIO : http://catch22.eu/beaglebone/beaglebone-pru-uio/
