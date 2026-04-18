CC = gcc
LD = ld
AS = fasm

CFLAGS = -m64 \
-ffreestanding \
-fno-stack-protector \
-fno-pic \
-fno-pie \
-mno-red-zone \
-mcmodel=kernel 

LDFLAGS = -m elf_x86_64 \
-nostdlib \
-T kernel/linker.ld

OBJS = build/main.o build/entry.o build/uart.o build/string.o build/bump.o
GRUB_MODULES = part_gpt fat normal multiboot2 all_video

.PHONY: run clean

build/%.o: kernel/%.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< $@ # fasm... rly?


build/%.o: kernel/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/driver/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@


build/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

build/BOOTX64.EFI: grub/grub.cfg
	@mkdir -p build
	grub-mkstandalone \
	--format=x86_64-efi \
	--output=$@ \
	--modules="$(GRUB_MODULES)" \
	"boot/grub/grub.cfg=grub/grub.cfg"

build/esp.img: build/BOOTX64.EFI build/kernel.elf
	dd if=/dev/zero of=build/esp.img bs=1M count=64
	mformat -i build/esp.img -F -T 131072 -H 2048 ::
#	64Mb / 1sec=512b == 2^(26-9) == 2^(17) = 131072 is a sector count
# 	2048 * 1sector == 2048 * 512b == 2^20b == 1Mb. GPT
	mmd -i build/esp.img ::/EFI ::/EFI/BOOT ::/boot
	mcopy -i build/esp.img build/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i build/esp.img build/kernel.elf ::/boot/

build/usb.img: build/esp.img
	# create raw image
	dd if=/dev/zero of=$@ bs=1M count=128

	# make partition, GPT and esp
	parted -s $@ mklabel gpt
	parted -s $@ mkpart ESP fat32 1MiB 65MiB
	parted -s $@ set 1 esp on

	dd if=build/esp.img of=$@ bs=512 seek=2048 conv=notrunc

OVMF = /usr/share/ovmf/OVMF.fd
run: build/usb.img
	qemu-system-x86_64 \
	-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
	-drive format=raw,file=$< \
	-m 512M \
	-vga std \
	-serial stdio \
	-monitor vc \

clean:
	rm -rf build/


