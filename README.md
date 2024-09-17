# EDK2 UEFI Firmware for HTC HD2 (Leo) and HTC HD7 (Schubert)

## Status 

| Function      | Notes                                            |  LEO   | SCHUBERT |
|---------------|--------------------------------------------------|--------|-----------
| GPIO          | Based on cLK driver                              |   ✅   |    ✅    |
| SD Card       | Based on cLK driver                              |   ✅   |    ✅    |
| NAND          | Driver exists in cLK (probably won't be added)   |   ❌   |    ❌    |
| I2C           | Based on cLK driver                              |   ✅   |    ✅    |
| Panel         | Driver exists in cLK                             |   ❌   |    ❌    |
| Touchscreen   | Driver exists in linux                           |   ❌   |    ❌    |
| Charging      | Based on cLK code                                |   ✅   |    ❌    |
| Battery Gauge | Supported in cLK since 1.5.x                     |   ❌   |    ❌    |
| USB           | Driver exists in cLK                             |   ❌   |    ❌    |
| Keypad        | Loosely based on cLK driver                      |   ✅   |    ✅    |

## OS support notes
- Currently loading a close-to-mainline kernel is supported (see https://github.com/HTC-Leo-Revival-Project/linux)

- Windows RT isn't supported (the os doesn't have inbox support for interrupt controller and timer found on hd2)

- ReactOS support should be possible once the arm port will be fully bootable

- Any new kernel (downstream or other binaries) and ramdisk can be loaded from SD card. (WARNING WIP)
- Pace the kernel named zImage and ramdisk named initrd.img on the root of your SD card.
- If booting fails (error shown on screen/screen turning blue/backlight stays on but screen is blank) just try again a few times

## Loading
UEFI can be either chainloaded as a kernel from cLK, as well as flashed to boot directly from HSPL.
However flashing as a HSPL image on HD7 is more complicated

## Credits
 - Cotulla and DFT for the work on HD2
 - cedesmith for creating the lk port for Leo, kokotas on further work
 - n0d3 for the sdcard driver in cLK
 - imbushuo for creating PrimeG2Pkg
 - ivoszbg for Msm8916Pkg
 - winocm for the iPhone4Pkg
 - feherneoh for all the helpfull ideas and shared knowledge
 - Krzysztof Łukasz Necel & Tomasz Gajger for a Tetris EFI implementation (based on https://github.com/swmicro/Tetris)

## License
All code except drivers in `GplDrivers` directory is licensed under BSD 2-Clause. 
GPL Drivers are licensed under GPLv2 license.
