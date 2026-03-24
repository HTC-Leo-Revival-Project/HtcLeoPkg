cmake_minimum_required(VERSION 2.8)
project(libboot C ASM)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -Wno-format-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wl,-gc-sections -O0 -ggdb")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")

# main
add_library(boot
    boot.c
    cmdline.c
    qcdt.c
    cksum/crc32.c
    loaders/android.c
    loaders/efi.c
    loaders/elf.c
    loaders/gzip.c
    loaders/qcmbn.c
    loaders/zimage.c
    tagloaders/atags.c
    tagloaders/fdt.c
    tagloaders/qcdt.c

    platforms/posix/platform.c
)
include_directories(
    include
    include_private
    platforms/posix/include
)

add_executable(libboot_test
    test/main.c
)
target_link_libraries(libboot_test boot fdt z)
