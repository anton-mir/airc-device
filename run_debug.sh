#!/bin/bash
read -sp 'Password: ' userpassw
sshpass -p $userpassw scp cmake-build-debug/src/airc_dev.elf airc@opi:firmware
sshpass -p $userpassw ssh airc@opi "cd firmware && ./flash.sh airc_dev.elf"
sshpass -p $userpassw ssh airc@opi -L 3333:localhost:3333 openocd -f firmware/openocd_commands.cfg
