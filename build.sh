#!/bin/bash
# based on the instructions from edk2-platform
set -e
export PACKAGES_PATH=$PWD/../edk2:$PWD/../edk2-platforms:$PWD
export WORKSPACE=$PWD/workspace

AvailablePlatforms=("Leo" "Schubert" "Passion" "All")
IsValid=0

while getopts d: flag
do
    case "${flag}" in
        d)
			device=${OPTARG}
			;;
    esac
done

function _check_args() {
	local DEVICE="${1}"
	for Name in "${AvailablePlatforms[@]}"
	do
		if [ "${DEVICE}" == "${Name}" ]
		then
			IsValid=1
			break;
		fi
	done
}

function _clean() {
	for PlatformName in "${AvailablePlatforms[@]}"
	do
		if [ "${PlatformName}" != "All" ]; then
			rm -f "ImageResources/${PlatformName}/bootpayload.bin"
			rm -f "ImageResources/${PlatformName}/os.nb"
			rm -f "ImageResources/${PlatformName}/uefi.img"
			rm -f "ImageResources/${PlatformName}/"*.nbh
		fi
	done
	rm -f "BootShim/BootShim."{bin,elf}
	rm -rf "workspace/Build"
	echo "Artifacts removed"
}

function _patch_edk() {
	echo "Patching EDK2 Source"

	PATCH1_APPLIED=0
	PATCH2_APPLIED=0

	file="../edk2/BaseTools/Source/C/GenFw/Elf32Convert.c"
	# Check if R_ARM_PC13 already exists
	if ! grep -q "case R_ARM_PC13:" "${file}"
	then
		# Insert after R_ARM_REL32
		sed -i '/case R_ARM_REL32:/a\        case R_ARM_PC13:' "${file}"
		echo "Inserted 'case R_ARM_PC13:' after 'case R_ARM_REL32:'."
		PATCH1_APPLIED=1
	else
		echo "'case R_ARM_PC13:' already exists. No changes made."
	fi


	file="../edk2/MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/UnicodeCollationEng.c"
	# Use sed to replace only if the "Status;" version exists
	if grep -q "EFI_STATUS  Status;" "${file}"
	then
		sed -i 's/EFI_STATUS  Status;/EFI_STATUS  Status = EFI_SUCCESS;/' "${file}"
		echo "Replaced 'EFI_STATUS  Status;' with 'EFI_STATUS  Status = EFI_SUCCESS;'"
		PATCH2_APPLIED=1
	else
		echo "Pattern not found, no changes made."
	fi

	if [ ${PATCH1_APPLIED} -eq 1 ] && [ ${PATCH2_APPLIED} -eq 1 ]
	then
		echo "Both patches applied successfully."
	fi
}

# based on https://github.com/edk2-porting/edk2-msm/blob/master/build.sh#L47 
function _build() {
	local DEVICE="${1}"
	shift
	source "../edk2/edksetup.sh"

	NUM_CPUS=$((`getconf _NPROCESSORS_ONLN` + 2))

	make clean -C ../edk2/BaseTools
	make -C ../edk2/BaseTools -j$(nproc)

	platforms=()

	if [ "${DEVICE}" == 'All' ]
	then
		for PlatformName in "${AvailablePlatforms[@]}"
		do
			if [ "${PlatformName}" != 'All' ]
			then
				platforms+=("${PlatformName}")
			fi
		done
	else
		platforms+=("${DEVICE}")
	fi

	for PlatformName in "${platforms[@]}"
	do
		echo "Building uefi for ${PlatformName}"
		build -n "${NUM_CPUS}" -a ARM -t CLANGDWARF -p "Platforms/Htc${PlatformName}/Htc${PlatformName}Pkg.dsc" -b DEBUG

		./build_boot_shim.sh
		./build_boot_images.sh "${PlatformName}"
	done
}

_check_args "${device}"
if [ $IsValid == 1 ]
then
	_clean
	_patch_edk
	_build "${device}"
else
	echo "Build: Invalid platform"
	echo "Available targets: "
	for Name in "${AvailablePlatforms[@]}"
	do
		echo " - ${Name}"
	done
fi
