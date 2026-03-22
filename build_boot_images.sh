#!/bin/bash
if [ $1 == 'Leo' ]; then
    cat BootShim/BootShim.bin workspace/Build/HtcLeo/DEBUG_CLANGDWARF/FV/QSD8250_UEFI.fd >>ImageResources/Leo/bootpayload.bin

    mkbootimg --kernel ImageResources/Leo/bootpayload.bin --base 0x11800000 --kernel_offset 0x00008000 --header_version 0 --cmdline "androidboot.hardware=leo" -o ImageResources/Leo/uefi.img

    # NBH creation
    if [ ! -f ImageResources/Leo/nbgen ]; then
        gcc -std=c99 ImageResources/Leo/nbgen.c -o ImageResources/Leo/nbgen
    fi
    # We're using a prebuild yang binary for now, since the compiled version doesn't seem to produce a valid NBH for Leo
    if [ ! -f ImageResources/Leo/yang ]; then
        gcc ImageResources/Leo/yang/nbh.c ImageResources/Leo/yang/nbhextract.c ImageResources/Leo/yang/yang.c -o ImageResources/Leo/yangbin
    fi

    cd ImageResources/Leo/
    ./nbgen os.nb
    ./yang -F LEOIMG.nbh -f logo.nb,os.nb -t 0x600,0x400 -s 64 -d PB8110000 -c 11111111 -v EDK2 -l WWE
    cd ../../
elif [ $1 == 'Schubert' ]; then
    cat BootShim/BootShim.bin workspace/Build/Htc$1/DEBUG_CLANGDWARF/FV/QSD8250_UEFI.fd >>ImageResources/$1/bootpayload.bin

    ./ImageResources/mkbootimg/mkbootimg.py --kernel ImageResources/$1/bootpayload.bin --base 0x20000000 --kernel_offset 0x00008000 --header_version 0 --cmdline "androidboot.hardware=schubert" -o ImageResources/$1/uefi.img
elif [ $1 == 'Passion' ]; then
    cat BootShim/BootShim.bin workspace/Build/Htc$1/DEBUG_CLANGDWARF/FV/QSD8250_UEFI.fd >>ImageResources/$1/bootpayload.bin

   ./ImageResources/mkbootimg/mkbootimg.py --kernel ImageResources/$1/bootpayload.bin --ramdisk ImageResources/$1/ramdisk --base 0x20000000 --kernel_offset 0x00008000 --header_version 0 --cmdline "androidboot.hardware=passion" -o ImageResources/$1/uefi.img
else
    echo "Bootimages: Invalid platform"
fi