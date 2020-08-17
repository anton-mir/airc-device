cmake_minimum_required(VERSION 3.0)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
project(wh1602 NONE)

set(LOCAL_SRCS "")
set(LOCAL_HDRS "")

#set(CMAKE_C_COMPILER "$ENV{CROSS_COMPILE}gcc")

add_definitions(-DHSE_VALUE=8000000U)
add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DDEBUG)
add_definitions(-DSTM32F407xx)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -std=gnu11 -g3 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")

file(GLOB LOCAL_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.h")

set(LOCAL_SRCS
    "wh1602.c")

include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../CMSIS/core/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../dev/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../hal/inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../../../inc")

add_library(wh1602 ${LOCAL_SRCS} ${LOCAL_HDRS})

