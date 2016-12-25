#!/bin/bash

# Set the governor in the kernel to "performance"
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Setup our output GPIOs for the PRU (0[30], 0[31])
echo 30 > /sys/class/gpio/export
echo 31 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio30/direction
echo out > /sys/class/gpio/gpio31/direction 

# Setup our output GPIOs for the cartridge display (1[16], 0[14])
echo 48 > /sys/class/gpio/export
echo 14 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio48/direction
echo out > /sys/class/gpio/gpio14/direction

# Load /dev/dsp support
modprobe -q snd_pcm_oss

# Give the network a few seconds to get going
sleep 7

# Start BES
cd /home/ubuntu/bes
OUR_IP="$(hostname -I)"


export SDL_AUDIODRIVER=dsp
nice -n -20 ./bes "${OUR_IP}"

