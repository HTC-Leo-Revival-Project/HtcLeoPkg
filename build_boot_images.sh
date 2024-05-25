#!/bin/bash
if [ $1 == 'Leo' ]; then
    cat BootShim/BootShim.bin workspace/Build/HtcLeo/DEBUG_GCC/FV/QSD8250_UEFI.fd >>ImageResources/Tools/bootpayload.bin

    mkbootimg --kernel ImageResources/Tools/bootpayload.bin --base 0x11800000 --kernel_offset 0x00008000 -o ImageResources/leo_uefi.img

    # NBH creation
    if [ ! -f ImageResources/Tools/nbgen ]; then
        gcc -std=c99 ImageResources/Tools/nbgen.c -o ImageResources/Tools/nbgen
    fi
    # We're using a prebuild yang binary for now, since the compiled version doesn't seem to produce a valid NBH for Leo
    if [ ! -f ImageResources/Tools/yang ]; then
        gcc ImageResources/Tools/yang/nbh.c ImageResources/Tools/yang/nbhextract.c ImageResources/Tools/yang/yang.c -o ImageResources/Tools/yangbin
    fi

    cd ImageResources/Tools
    ./nbgen os.nb
    mv ./os.nb ./os_leo.nb
    ./yang -F ../LEOIMG.nbh -f logo.nb,os_leo.nb -t 0x600,0x400 -s 64 -d PB8110000 -c 11111111 -v EDK2 -l WWE
    cd ../../
elif [ $1 == 'Schubert' ]; then
    cd WpShim && make UEFI_BASE=0x24000000 UEFI_SIZE=0x00100000 && cd ../
    cat WpShim/BootShim.bin workspace/Build/HtcSchubert/DEBUG_GCC/FV/QSD8250_UEFI.fd >>ImageResources/Schubert/bootpayload.bin

    # NBH creation
    # cd ImageResources/Schubert && ./makenbh.sh

elif [ $1 = "Passion" ] || [ $1 = "Bravo" ]; then
    cat BootShim/BootShim.bin workspace/Build/Htc$1/DEBUG_GCC/FV/QSD8250_UEFI.fd >>ImageResources/$1/bootpayload.bin
    mkbootimg --kernel ImageResources/$1/bootpayload.bin --ramdisk ImageResources/$1/dummy --base 0x20000000 --kernel_offset 0x00008000 -o ImageResources/${1,,}_uefi.img
else
    echo "Bootimages: Invalid platform"
fi
