#!/bin/bash
scp -P 5025 build/src/airc_dev.elf airc@176.37.42.185:firmware
ssh -p 5025 airc@176.37.42.185 "cd firmware && ./flash.sh airc_dev.elf"
ssh -p 5025 airc@176.37.42.185 -L 3333:localhost:3333 openocd -f firmware/openocd_commands.cfg
