mkdir build -p && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake .. && make -j$(nproc)
