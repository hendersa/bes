#!/bin/bash

# Set the governor in the kernel to "performance"
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Start our web interface
cd /home/ubuntu/bes-config-node
#./run.sh > /dev/null

# Start BES
cd /home/ubuntu/bes
OUR_IP="$(hostname -I)"

echo "${OUR_IP}"

modprobe -q snd_pcm_oss
#modprobe -q pruss_remoteproc

export SDL_AUDIODRIVER=dsp
#nice -n -20 ./bes "${OUR_IP}" 
./bes "${OUR_IP}

