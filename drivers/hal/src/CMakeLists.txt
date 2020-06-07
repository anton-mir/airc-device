cmake_minimum_required(VERSION 3.0)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
project(stm32f407hal NONE)

set(LOCAL_SRCS "")
set(LOCAL_HDRS "")

#set(CMAKE_C_COMPILER "$ENV{CROSS_COMPILE}gcc")

add_definitions(-DHSE_VALUE=8000000U)
add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DDEBUG)
add_definitions(-DSTM32F407xx)
add_definitions(-DETH_PHY_KSZ8081RND)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -std=gnu11 -g3 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")

# file(GLOB LOCAL_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB LOCAL_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.h")
file(GLOB LOCAL_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/../inc/Legacy/*.h")

set(LOCAL_SRCS
    "stm32f4xx_hal.c"
    "stm32f4xx_hal_cortex.c"
    "stm32f4xx_hal_crc.c"
    "stm32f4xx_hal_dma.c"
    "stm32f4xx_hal_dma_ex.c"
    "stm32f4xx_hal_eth_ksz8081.c"
    "stm32f4xx_hal_exti.c"
    "stm32f4xx_hal_flash.c"
    "stm32f4xx_hal_flash_ex.c"
    "stm32f4xx_hal_flash_ramfunc.c"
    "stm32f4xx_hal_gpio.c"
    "stm32f4xx_hal_msp_template.c"
    "stm32f4xx_hal_pwr.c"
    "stm32f4xx_hal_pwr_ex.c"
    "stm32f4xx_hal_rcc.c"
    "stm32f4xx_hal_rcc_ex.c"
    "stm32f4xx_hal_sram.c"
    "stm32f4xx_hal_tim.c"
    "stm32f4xx_hal_tim_ex.c"
    "stm32f4xx_hal_rng.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../CMSIS/core/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../dev/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../inc/Legacy")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../../inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../ksz8081rnd/inc")

add_library(stm32f407hal ${LOCAL_SRCS} ${LOCAL_HDRS})

