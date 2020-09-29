cmake_minimum_required(VERSION 3.0)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
project(airc_dev.elf NONE)

set(LOCAL_SRCS "")
set(LOCAL_HDRS "")

add_definitions(-DHSE_VALUE=8000000U)
add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DDEBUG)
add_definitions(-DSTM32F407xx)
add_definitions(-DETH_PHY_KSZ8081RND)

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/lwip)
set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../inc/arch")

include(${LWIP_DIR}/src/Filelists.cmake)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../linker/STM32F407VGTX_FLASH.ld")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -std=gnu11 -g3 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")

file(GLOB LOCAL_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.h")

set(HAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../drivers/hal)

set(LOCAL_SRCS
    "stm32f4xx_it.c"
    "${HAL_DIR}/src/stm32f4xx_hal_uart.c"
    "system_stm32f4xx.c"
    "ethernetif.c"
    "esp8266_wifi.c"
    "http_helper.c"
    "picohttpparser.c"
    "uart_sensors.c"
	"bmp280.c"
    "syscalls.c"
    "sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/arch/sys_arch.c"
	"lm335z.c"
    "main.c"
    	"i2c_ccs811sensor.c"
	"eth_server.c"
	"eth_sender.c"
	"data_collector.c"
    "leds.c"
	"config_board.c"
	"flash_SST25VF016B.c"
	"${HAL_DIR}/src/stm32f4xx_hal_spi.c"
    "stm32f4xx_hal_msp.c"
    "number_helper.c"
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../drivers/CMSIS/core/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../drivers/dev/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../drivers/hal/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../lib/FreeRTOS-Kernel/include")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../lib/FreeRTOS-Kernel/portable/GCC/ARM_CM4F")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../drivers/ksz8081rnd/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../drivers/wh1602/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../lib/lwip/src/include")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../lib/lwip/src/include")
# include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../inc/arch")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../inc")

set (CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} -Wl,-Map=${PROJECT_NAME}.map -Wl,--gc-sections -Wl,--start-group -Wl,-lc -Wl,-lm -Wl,--end-group")

add_executable(${PROJECT_NAME} ${LOCAL_SRCS} ${LOCAL_HDRS})

target_include_directories(airc_dev.elf PRIVATE ${LWIP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} stm32f407hal freertos ksz8081rnd wh1602 lwipcore startup)

