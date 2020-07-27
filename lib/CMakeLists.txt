cmake_minimum_required(VERSION 3.0)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
project(freertos NONE)

set(LOCAL_SRCS "")
set(LOCAL_HDRS "")

#set(CMAKE_C_COMPILER "$ENV{CROSS_COMPILE}gcc")

add_definitions(-DHSE_VALUE=8000000U)
add_definitions(-DUSE_HAL_DRIVER)
add_definitions(-DDEBUG)
add_definitions(-DSTM32F407xx)

#message(STATUS "!!! CURRENT SOURCE DIR: " "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -std=gnu11 -g3 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")

file(GLOB LOCAL_HDRS "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/include/*.h")

set(LOCAL_HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/portmacro.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../inc/FreeRTOSConfig.h"
)

set(LOCAL_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/croutine.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/event_groups.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/list.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/queue.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/stream_buffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/tasks.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/timers.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/portable/GCC/ARM_CM4F/port.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel/portable/MemMang/heap_1.c"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} "../inc")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "FreeRTOS-Kernel/include")
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "FreeRTOS-Kernel/portable/GCC/ARM_CM4F")

add_library(freertos ${LOCAL_SRCS} ${LOCAL_HDRS})

