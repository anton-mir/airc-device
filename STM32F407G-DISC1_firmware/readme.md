# AirC device repo

Software for flashing STM32F407G DISCOVERY with openocd.

### Install openocd
```
$ sudo apt install openocd
```
### Flash with openocd

Be sure you have built and copied firmware 'airc_dev.elf' here, to the same folder with openocd config 'openocd.cfg'. 

Flash with:
``` 
$ sudo openocd
```

### Debug with openocd

```
$ openocd -f openocd_commands.cfg
```
#### Run gdb locally
Install GDB
```
$ sudo apt-get install gdb-multiarch
```
Debug with GDB:
```
$ gdb-multiarch
(gdb) file airc_dev.elf
(gdb) target extended-remote localhost:3333
```
If needed, kill openocd process with "sudo pkill -9 openocd"

