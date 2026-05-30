CC = gcc
AS = fasm
LD = ld

BUILD_DIR ?= ../build/user
ULIB = $(BUILD_DIR)/usys.o $(BUILD_DIR)/ulib.o $(BUILD_DIR)/umalloc.o

CFLAGS = -m64 \
-ffreestanding \
-fno-stack-protector \
-fno-pic \
-fno-pie \
-mno-red-zone \
-g

LDFLAGS = -m elf_x86_64 \
-nostdlib \

UPROGS = init sh cat mkdir ls rm nsh

all: $(BUILD_DIR)/initcode $(UPROGS:%=$(BUILD_DIR)/%)


# $(UPROGS:%=$(BUILD_DIR)/%)
# for each word in UPROGS, prepend BUILD_DIR

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/initcode.out: $(BUILD_DIR)/initcode.o
	$(LD) -nostdlib -N -e _start -Ttext 0 -o $@ $<

$(BUILD_DIR)/initcode: $(BUILD_DIR)/initcode.out
	objcopy -S -O binary $< $@
# -S: strip symbols/debug info, -O binary: make the files like it is loaded on the mem

# 	build/user/initcode: build/user/initcode.out
# 	objcopy -S -O binary $< $@

# build/user/initcode.out: build/user/initcode.o
# 	$(LD) -nostdlib -N -e _start -Ttext 0 -o $@ $<

$(BUILD_DIR)/% : $(BUILD_DIR)/%.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0x1000 -o $@ $^
