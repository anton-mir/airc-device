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
$ /opt/st/stm32cubeide_1.2.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_1.1.0.201910081157/tools/bin/STM32_Programmer_CLI -c port=SWD -d airc_dev.elf
```
Also you might install cubeprogrammer from stm website and use STM32_Programmer_CLI from installed path.

### How to flash and test at remote
```
$ cd ~/src/GL-SMARTCITY/sbc-platform/src/airc-device
$ scp -P 5025 ./build/src/airc_dev.elf airc@[IP_ADDRESS]:/home/airc/firmware 
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
Flash **stm32eth.elf** file to your STM32F4DISCOVERY as described earlier.

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


