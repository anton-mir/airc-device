# AirC device repo

Source code for STM32F4DISCOVERY AirC project.

### How to setup cross-compile toolchain
```
$ cd ~ && mkdir toolchain && cd toolchain
$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
$ bzip2 -d gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
$ tar -xf gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar
$ cd  ~/src/GL-SMARTCITY/sbc-platform/src/airc-device
$ git submodule update --init
$ source env.src
$ ./build.sh
```
### How to flash locally

If you have installed CubeIDE, then:
```
$ cd ~/src/GL-SMARTCITY/sbc-platform/src/airc-device/build/src
$ /opt/st/stm32cubeide_1.2.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_1.1.0.201910081157/tools/bin/STM32_Programmer_CLI -c port=SWD -d [firmware_name].elf
```
Also you might install cubeprogrammer from stm website and use STM32_Programmer_CLI from installed path.

### How to flash and test at remote
```
$ cd ~/src/GL-SMARTCITY/sbc-platform/src/airc-device
$ scp -P 5025 ./build/src/[firmware_name].elf airc@[IP_ADDRESS]:/home/airc/firmware 
$ ssh airc@[IP_ADDRESS] -p 5025 
$ cd firmware 
$ sudo openocd
```
### How to build test app "stm32eth echo"

```
$ git checkout dev_test_sockets
$ source env.src
$ ./build.sh 
```
Flash **[firmware_name].elf** file to your STM32F4DISCOVERY as described earlier.

Connect GL Starter Kit with ethernet cable to router or directly to your host PC (use linux OS).
```
$ sudo apt-get install nc
$ nc [IP_ADDRESS_FROM_DISPLAY] 11333
```
Type something and you should see Starter Kit reply back same message to you.

### How to setup Clion IDE to build project
```
$ echo 'export PATH=~/toolchain/gcc-arm-none-eabi-9-2020-q2-update/bin:$PATH' >> ~/.bashrc
```

Goto **Settings->Build, execution, deployment->CMake**

CMake options:

`-DCMAKE_TOOLCHAIN_FILE=/home/[USERNAME]/src/GL-SMARTCITY/sbc-platform/src/airc-device/toolchain.cmake`

Environment: 

`CROSS_COMPILE=/home/[USERNAME]/toolchain/gcc-arm-none-eabi-9-2020-q2-update/bin/arm-none-eabi-`

### Debug remotelly with ssh port-forwarding

#### Upload firmware
```
$ scp -P 5025 ~/src/GL-SMARTCITY/sbc-platform/src/airc-device/build/src/[firmware_name].elf airc@176.37.42.185:firmware 
```
#### Flash firmware
```
$ ssh airc@176.37.42.185 -p 5025 "cd firmware && ./flash.sh [firmware_name].elf"
```
#### Run openocd at remote
```
$ ssh airc@176.37.42.185 -p 5025 -L 3333:localhost:3333 openocd -f firmware/openocd_commands.cfg
```
#### Run gdb locally
```
$ sudo apt-get insatll gdb-multiarch
$ gdb-multiarch
(gdb) file [firmware_name].elf
(gdb) target extended-remote localhost:3333
```
If needed, kill openocd process with "ssh airc@176.37.42.185 -p 5025 ./firmware/stop_debug.sh"

# WiFi

To provide wifi connection used ESP8266: http://www.microchip.ua/wireless/esp01.pdf
  - Required AT firmware: v0.22.
  - Required SDK: 1.0.0.
  - Required boot loader: v1.3.

**PINS connection**
   - VCC -> 3.3v.
   - GND -> GND.
   - CH_PD(EN) -> 3.3v.
   - RX -> PC6.
   - TX -> PC7.

**Soft AP default credentials**
   - SSID: **AirC Device**.
   - PASS: **314159265**.

**Internal web-server information**
   - Settings address: **http://192.168.4.1/settings**.
