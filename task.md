# 0 Prepare

- Change time to Stockholm
- _ssh-keygen -t ecds_
- Add key to your Github
- Pull this instruction from repo
- Create a branch in airc-device repo with your name in it and use it

# 1 Build and flash

* _./build.sh_
* _openocd_

# 1 Check that packet arrives

Connect GL board with Ethernet cable and establish TCP/IP connection:

* _telnet 192.168.1.121 11333_
or
* _nc 192.168.1.121 11333_

Try to check packet with running "_wireshark_"

# 2 Connect LED

Blue led is PB14, see "_AirC sensors and pinout specification.ods_" in **./docs** and docs in **./pcb**
Observe it blinking.

# 3 Run Debugger

1. Run openocd server
2. Run debug with config "(gdb) Start airc debug"
3. Find RTOS tasks in main.c from line 144

Walk through code in debug and try to understand what is going on.
For this set Break Points inside each task's code, pay attention to loops:

- uart_sensors loop - sensors read function
- bme280_sensor
- i2c_ccs811sensor
- analog_temp
- data_collector
- eth_sender
- reed_switch_task
- display_data_task

# 4 Try modify led *

Try to use wire instead of reed swith to modify working -> wifi mode and lit green led,
connected to other board pin, see ***leds.c*** and pinout in "_AirC sensors and pinout specification.ods_" 

# 5 Mock data from UART sensors **

See "_void uart_sensors(void * const arg)_" in **uart_sensors.c**

**Data structures declaration:** 

```
double pm2_5_val = 0;
double pm10_val = 0;
double HCHO_val = 0;
```
```
struct SPEC_values
{
    unsigned long long int specSN;
    long long specPPB;
    unsigned long int specTemp, specRH, specDay, specHour, specMinute, specSecond;
    int8_t error_reason;
};
```

Mock and observe fake data at the display.

# Mock other sensors ***

DIY

# Merge request create

Push your branch
Create merge request towards master