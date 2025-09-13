#!/bin/bash
# based on the instructions from edk2-platform
set -e
export PACKAGES_PATH=$PWD/../edk2:$PWD/../edk2-platforms:$PWD
export WORKSPACE=$PWD/workspace

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

	NUM_CPUS=$((`getconf _NPROCESSORS_ONLN` + 2))

	# Clean artifacts if needed
	_clean
	echo "Artifacts removed"
if [ $DEVICE == 'All' ]; then
    echo "Building uefi for all platforms"
	for PlatformName in "${AvailablePlatforms[@]}"
	do
		if [ $PlatformName != 'All' ]; then
			# Build
			build -n $NUM_CPUS -a ARM -t CLANGDWARF -p Platforms/Htc${PlatformName}/Htc${PlatformName}Pkg.dsc -b DEBUG
			./build_boot_shim.sh
			./build_boot_images.sh $PlatformName
		fi
	done
else

	echo "Patching EDK2 Source"
file="../edk2/BaseTools/Source/C/GenFw/Elf32Convert.c"

# Check if R_ARM_PC13 already exists
if ! grep -q "case R_ARM_PC13:" "$file"; then
    # Insert after R_ARM_REL32
    sed -i '/case R_ARM_REL32:/a\        case R_ARM_PC13:' "$file"
    echo "Inserted 'case R_ARM_PC13:' after 'case R_ARM_REL32:'."
else
    echo "'case R_ARM_PC13:' already exists. No changes made."
fi


file="../edk2/MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/UnicodeCollationEng.c"

# Use sed to replace only if the "Status;" version exists
if grep -q "EFI_STATUS  Status;" "$file"; then
    sed -i 's/EFI_STATUS  Status;/EFI_STATUS  Status = EFI_SUCCESS;/' "$file"
    echo "Replaced 'EFI_STATUS  Status;' with 'EFI_STATUS  Status = EFI_SUCCESS;'"
else
    echo "Pattern not found, no changes made."
fi

    echo "Building uefi for $DEVICE"
	build -n $NUM_CPUS -a ARM -t CLANGDWARF -p Platforms/Htc${DEVICE}/Htc${DEVICE}Pkg.dsc -b DEBUG

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
