#
#  Copyright (c) 2018, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = HtcSchubert
  PLATFORM_GUID                  = 28f1a3bf-193a-47e3-a7b9-5a435eaab2ef
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = Platforms/$(PLATFORM_NAME)/$(PLATFORM_NAME)Pkg.fdf

  DEFINE USE_SCREEN_FOR_SERIAL_OUTPUT = TRUE

!include QSD8250Pkg/CommonDsc.dsc.inc

[BuildOptions.common]
  GCC:*_*_ARM_CC_FLAGS = -DKP_LED_ENABLE_METHOD=3 -DDEVICETYPE=2 # Gpio

[PcdsFixedAtBuild.common]
  # System Memory (243MB for now)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x20000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x0E800000

  gQSD8250PkgTokenSpaceGuid.PcdMipiFrameBufferAddress|0x02A00000

  gQSD8250PkgTokenSpaceGuid.PcdMachType|0x9DC

  # SMBIOS
  gQSD8250PkgTokenSpaceGuid.PcdSmbiosSystemModel|"HTC HD7"
  gQSD8250PkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"HTC SCHUBERT"