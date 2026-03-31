ARCH              = arm
TARGET            = arm-none-eabi
CROSS_COMPILE     = $(TARGET)-
CC                = $(CROSS_COMPILE)gcc
OBJCOPY           = $(CROSS_COMPILE)objcopy

all: Payload.elf Payload.bin

Payload.bin: Payload.elf
	$(OBJCOPY) -O binary $< $@
	rm -f $<

Payload.elf: Payload.S
	$(CC) -c $< -o $@ -mfpu=vfpv3 -mfloat-abi=hard -DUEFI_BASE=$(UEFI_BASE) -DUEFI_SIZE=$(UEFI_SIZE)

Payload.S: