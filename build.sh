#!/bin/bash
# based on the instructions from edk2-platform
set -e
export PACKAGES_PATH=$PWD/../edk2:$PWD/../edk2-platforms:$PWD
export WORKSPACE=$PWD/workspace
export GCC_ARM_PREFIX=arm-linux-gnueabihf-

AvailablePlatforms=("Leo" "Schubert" "All")
IsValid=0

while getopts d: flag
do
    case "${flag}" in
        d) device=${OPTARG};;
    esac
done

function _check_args(){
	local DEVICE="${1}"
	for Name in "${AvailablePlatforms[@]}"
	do
		if [[ $DEVICE == "$Name" ]]; then
			IsValid=1
			break;
		fi
	done
}

function _clean() {
	for PlatformName in "${AvailablePlatforms[@]}"
		do
			if [ $PlatformName != 'All' ]; then
				if [ -f ImageResources/$DEVICE/bootpayload.bin ]; then
					rm ImageResources/$DEVICE/bootpayload.bin
				fi
				if [ -f ImageResources/$DEVICE/os.nb ]; then
					rm ImageResources/$DEVICE/os.nb
				fi
				if [ -f ImageResources/$DEVICE/*.img ]; then
					rm ImageResources/$DEVICE/*.img
				fi
			fi
		if [ -f BootShim/BootShim.bin ]; then
			rm BootShim/BootShim.bin
		fi
		if [ -d workspace/Build ]; then
			rm -r workspace/Build
		fi
	done
}

# based on https://github.com/edk2-porting/edk2-msm/blob/master/build.sh#L47 
function _build(){
	local DEVICE="${1}"
	shift
	source "../edk2/edksetup.sh"

	# Clean artifacts if needed
	_clean
	echo "Artifacts removed"
if [ $DEVICE == 'All' ]; then
    echo "Building uefi for all platforms"
	for PlatformName in "${AvailablePlatforms[@]}"
	do
		if [ $PlatformName != 'All' ]; then
			# Build
			build -s -n 0 -a ARM -t GCC -p Platforms/Htc${PlatformName}/Htc${PlatformName}Pkg.dsc
			./build_boot_shim.sh
			./build_boot_images.sh $PlatformName
		fi
	done
else
    echo "Building uefi for $DEVICE"
	build -s -n 0 -a ARM -t GCC -p Platforms/Htc${DEVICE}/Htc${DEVICE}Pkg.dsc

	./build_boot_shim.sh
	./build_boot_images.sh $DEVICE
fi
}

_check_args "$device"
if [ $IsValid == 1 ]; then
	_build "$device"
else
	echo "Build: Invalid platform"
	echo "Available targets: "
	for Name in "${AvailablePlatforms[@]}"
	do
		echo " - "$Name
	done
fi
