#!/bin/bash
read -sp 'Password: ' userpassw
sshpass -p $userpassw ssh airc@opi "cd firmware && rm *.elf && ls -la"
sshpass -p $userpassw scp cmake-build-debug/src/stm32eth.elf airc@opi:firmware
sshpass -p $userpassw ssh airc@opi "cd firmware && ls && ./flash.sh stm32eth.elf"
sshpass -p $userpassw ssh airc@opi -L 3333:localhost:3333 openocd -f firmware/openocd_commands.cfg