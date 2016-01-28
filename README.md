# INMOS-ispy

A slightly updated version of the old INMOS Transputer 'ispy' and 'mtest' tools for basic configuration and diagnostics of INMOS Transputer hardware on Linux systems.

This set of tools has been modified slightly from the last release by  Andy Rabagliati (http://www.wizzy.com/wizzy/ispy.html), in order to work with modern versions of GCC and the updated kernel driver on Linux 3.x.x as available in my other repository (https://github.com/megatron-uk/INMOS-Link-Driver).

## Useage

Should build cleanly with GNU make and any modern version of GCC.

ispy and mtest work as previous - anyone who is used to the old versions should find no substantial changes in behaviour. Most modifications are limited to code formatting and header file locations.

## Notes

Supplied with a copy of link-driver.h as bundled with the kernel driver above. I will attempt to keep the two header files in sync, but if in doubt, install the latest kernel driver from my repo and symlink it here instead.
