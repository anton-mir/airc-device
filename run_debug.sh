#!/bin/bash
ssh airc@opi -L 3333:localhost:3333 openocd -f firmware/openocd_commands.cfg