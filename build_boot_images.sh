#!/usr/bin/bash

device="${1}"
devicelower="$(tr [:upper:] [:lower:] <<< "${device}")"

kernelbase=0x20000000

mkdir -p "ImageResources/${device}"

cat "BootShim/BootShim.bin" "workspace/Build/Htc${device}/DEBUG_CLANGDWARF/FV/QSD8250_UEFI.fd" > "ImageResources/${device}/bootpayload.bin"

case "${device}" in
    Leo)
        kernelbase=0x11800000

        cd ImageResources/Leo/
        ./nbgen os.nb
        if [ $? -ne 0 ]
        then
            echo "Failed to build os.nb for ${device}" 1>&2
            exit 1
        fi
        ./yang -F LEOIMG.nbh -f logo.nb,os.nb -t 0x600,0x400 -s 64 -d PB8110000 -c 11111111 -v EDK2 -l WWE
        if [ $? -ne 0 ]
        then
            echo "Failed to build LEOIMG.nbh" 1>&2
            exit 1
        fi
        cd ../../
        ;;
    Passion|Schubert)
        ;;
    *)
        echo "Bootimages: Invalid platform: ${device}" 1>&2
        exit 1
        ;;
esac

ramdisk="$(mktemp)"
printf "\0" > "${ramdisk}"
./ImageResources/mkbootimg/mkbootimg.py --kernel "ImageResources/${device}/bootpayload.bin" --ramdisk "${ramdisk}" --base "${kernelbase}" --kernel_offset 0x00008000 --header_version 0 --cmdline "androidboot.hardware=${devicelower}" -o "ImageResources/${device}/uefi.img"
ret=$?
if [ ${ret} -ne 0 ]
then
    echo "Failed to build uefi.img for ${device}" 1>&2
    exit 1
fi
rm -f "${ramdisk}"
exit ${ret}
