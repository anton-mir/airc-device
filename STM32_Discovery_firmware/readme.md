# AirC device repo

Software for flashing STM32F407G DISCOVERY with openocd.

### Install openocd
```
$ sudo apt install openocd
```
### Flash with openocd 

Flash with:
``` 
$ sudo openocd
```

#### Debug with gdb

Install GDB
```
$ sudo apt-get install gdb-multiarch
```
Copy airc_dev.elf to the current folder.
Run openocd in other terminal window:

```
$ openocd -f openocd_commands.cfg
```

Run GDB and debug:

```
$ gdb-multiarch
(gdb) file airc_dev.elf
(gdb) target extended-remote localhost:3333
(dbg) ...following debug commands...
```
If needed, kill openocd process with "sudo pkill -9 openocd"

#### Debug with VSCode

Run openocd in other terminal window:

```
$ openocd -f openocd_commands.cfg
```

Use following debug configuration for debug (.vscode/launch.json):

```
{
    "name": "(gdb) Start airc debug",
    "type": "cppdbg",
    "cwd": "${workspaceFolder}",
    "request": "launch",
    "targetArchitecture": "arm64",
    "useExtendedRemote": true,
    "program": "${workspaceFolder}/build/src/airc_dev.elf",
    "linux": {
        "MIMode": "gdb",
        "miDebuggerPath": "/usr/bin/gdb-multiarch",
        "miDebuggerServerAddress": "localhost:3333"
    },
    "setupCommands": [
        {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        },
        {
            "description": "Set Disassembly Flavor to Intel",
            "text": "-gdb-set disassembly-flavor intel",
            "ignoreFailures": true
        }
    ]
}
```
