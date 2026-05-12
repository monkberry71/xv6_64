CC = gcc
AS = fasm
LD = ld

UPROGS = _sh

BUILD_DIR ?= ../build/user
ULIB = $(BUILD_DIR)/usys.o

CFLAGS = -m64 \
-ffreestanding \
-fno-stack-protector \
-fno-pic \
-fno-pie \
-mno-red-zone \
-mcmodel=kernel \
-g

LDFLAGS = -m elf_x86_64 \
-nostdlib \
-T kernel/linker.ld

all: $(UPROGS:%=$(BUILD_DIR)/%)

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/_% : $(BUILD_DIR)/%.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
