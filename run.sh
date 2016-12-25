#!/bin/sh

# Start BES
cd /home/ubuntu/bes
OUR_IP="$(hostname -I)"

export SDL_AUDIODRIVER=dsp
nice -n -20 ./bes "${OUR_IP}"

