# AirC device repo

This repo is a part of the GlobalLogic AirC device POC and hosts source code for STM32F4DISCOVERY AirC Device project.
Please check [GL-SMARTCITY repo](https://github.com/anton-mir/GL-SMARTCITY) for more info.


In order to build the project using this instructions please set up cross-compile toolchain first. 

### Setup cross-compile toolchain
```
$ cd ~ && mkdir toolchain && cd toolchain
$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
$ bzip2 -d gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
$ tar -xf gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar
$ cd  ~/src/GL-SMARTCITY/sbc-platform/src/airc-device
$ git submodule update --init
```
### Build project

Now you can build the project with the toolchain:
```
$ source env.src
$ ./build.sh
```

You can find firmware here **./build/src/airc_dev.elf**

### Flash the firmware

Connect STM32F4DISCOVERY board to computer. 
If you use Virtual Box, forward (connect) USB device (STM32F4DISCOVERY) to Virtual machine an be sure it is available
(if you use Windows Host you will need to install Virtual Box Extension Pack first and set up STM32F4DISCOVERY as
connected USB device in Virtual machine's settings). 

## Flash with openocd (default option)

```
$ cd ~/src/GL-SMARTCITY/sbc-platform/src/airc-device/STM32_Discovery_firmware
$ sudo openocd
```
This command will attempt to flash firmware (./build/src/airc_dev.elf) to connected STM32F4DISCOVERY board.
You can check openocd config (./STM32_Discovery_firmware/openocd.cfg) for the flashing options.

## Flash with STM32 tool

Install [STM32CubeProgrammer software for Linux V2.17.0](https://www.st.com/en/development-tools/stm32cubeprog.html#get-software)
```
$ cd ~/STMicroelectronics/STM32Cube/STM32CubeProgrammer217/bin
$ ./STM32_Programmer_CLI -c port=SWD -d ~/src/GL-SMARTCITY/sbc-platform/src/airc-device/build/src/airc_dev.elf
```
Or you can run STM32CubeProgrammer binary (~/STMicroelectronics/STM32Cube/STM32CubeProgrammer217/bin/STM32CubeProgrammer) and flash firmware with GUI.

### How to check TCP/IP packet from GL Starter Kit 

Connect GL STarter Kit to your computer with Ethernet cable.
Use **nc** or **telnet** to connect to GL Starter Kit and wait for the packet (by default packet is sent once every minute).

```
$ nc 192.168.1.121 11333 (or "telnet 192.168.1.121 11333")
Thu Aug 04 14:48:05 2016�(\��E:@@�@@@����
```
You can explore the packets from AirC Device with Wireshark by running:
```
$ sudo wireshark
```
### Debug AirC source code with VSCode

You may neeed following VSCode extentions installed:
* C/C++
* C/C++ Extension Pack
* CMake
* CMakeTools
* GDB Debug
* Remote - SSH
* Remote Explorer
* Remote - SSH: Editing Configuration Files

To debug with VSCode run openocd server in other terminal window:

```
$ openocd -f openocd_commands.cfg
```
Use "(gdb) Start airc debug" VSCode debug configuration.

You can also debug with GDB, see [readme.md in STM32_Discovery_firmware folder](STM32_Discovery_firmware/readme.md)

# Setup AirC Device sensor board configuration through ESP01 WiFi access point

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
