# How to use the built-in PWM on the BeagleBone Black

*Note: this was tested on this image:* `Linux beaglebone 4.4.91-ti-r133 #1 SMP Tue Oct 10 05:18:08 UTC 2017 armv7l GNU/Linux`

Based on this very useful link : https://www.teachmemicro.com/beaglebone-black-pwm-ubuntu-device-tree/

By default on this image, it shouldn't be required to load a special device tree to use the built-in PWM pin. We will use pin P9.14 in this example.

First, enable pwm mode on pin P9.14 by running this command :

    $ config-pin -a P9.14 pwm

Run the following command to check everything worked properly :

    $ config-pin -q P9.14
    P9_14 Mode: pwm

