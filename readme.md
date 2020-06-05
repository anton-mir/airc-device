To build:

$ cd ~ && mkdir toolchain && cd toolchain

$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2

$ bzip2 -d gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2

$ tar -xf gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar

$ cd  ~/src/GL-SMARTCITY/sbc-platform/src/airc-device

$ git submodule update --init

$ source env.src

$ ./build.sh