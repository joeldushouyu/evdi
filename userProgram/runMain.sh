#!/bin/sh
sudo renice -n -20 $$
./main | ts '[%Y-%m-%d %H:%M:%S]'
