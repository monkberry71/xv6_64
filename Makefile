CC = gcc
LD = ld
AS = fasm

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

USERS = build/user/initcode.o

build/user/%.o: user/%.asm
	@mkdir -p build/user
	$(AS) $(ASFLAGS) $< $@

build/user/initcode.out: build/user/initcode.o
	$(LD) -nostdlib -N -e _start -Ttext 0 -o $@ $<

build/user/initcode: build/user/initcode.out
	objcopy -S -O binary $< $@

OBJS = build/main.o build/entry.o build/uart.o build/string.o build/bump.o build/mb2.o \
build/debug.o build/vm.o build/kalloc.o build/mp.o build/vectors.o build/trap_asm.o build/trap.o \
build/gop.o build/lapic.o build/swtch.o build/spinlock.o build/proc.o build/cpu.o build/syscall.o \
build/syscall_entry.o build/sysproc.o build/bio.o build/ramdisk.o build/sleeplock.o
GRUB_MODULES = part_gpt fat normal multiboot2 all_video

.PHONY: run clean debug format_usb format_esp

build/%.o: kernel/%.asm
	@mkdir -p build
	$(AS) $(ASFLAGS) $< $@ # fasm... rly?


build/%.o: kernel/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/driver/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@


build/kernel.elf: $(OBJS) build/user/initcode
	$(LD) $(LDFLAGS) $(OBJS) --oformat elf64-x86-64 -b binary build/user/initcode -o $@ 

build/BOOTX64.EFI: grub/grub.cfg
	@mkdir -p build
	grub-mkstandalone \
	--format=x86_64-efi \
	--output=$@ \
	--modules="$(GRUB_MODULES)" \
	"boot/grub/grub.cfg=grub/grub.cfg"

build/esp.img:
	dd if=/dev/zero of=build/esp.img bs=1M count=64

format_esp: build/esp.img build/BOOTX64.EFI build/kernel.elf
	mformat -i build/esp.img -F -T 131072 -H 2048 ::
#	64Mb / 1sec=512b : build/e2b == 2^(26-9) == 2^(17) = 131072 is a sector count
# 	2048 * 1sector == 2048 * 512b == 2^20b == 1Mb. GPT
	mmd -i build/esp.img ::/EFI ::/EFI/BOOT ::/boot
	mcopy -i build/esp.img build/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i build/esp.img build/kernel.elf ::/boot/

build/usb.img:
	@mkdir -p build
	# create raw image
	dd if=/dev/zero of=$@ bs=1M count=128

format_usb: build/usb.img format_esp
	parted -s $< mklabel gpt
	parted -s $< mkpart ESP fat32 1MiB 65MiB
	parted -s $< set 1 esp on

	dd if=build/esp.img of=$< bs=512 seek=2048 conv=notrunc

OVMF = /usr/share/ovmf/OVMF.fd
run: format_usb
	qemu-system-x86_64 \
	-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
	-drive format=raw,file=build/usb.img \
	-m 512M \
	-vga std \
	-serial stdio \
	-d int,cpu_reset -D ./misc/qemu.log \
	-monitor vc \

debug: format_usb
	qemu-system-x86_64 \
	-drive if=pflash,format=raw,readonly=on,file=$(OVMF) \
	-drive format=raw,file=build/usb.img \
	-m 128M \
	-vga std \
	-serial stdio \
	-monitor vc \
	-s -S \
	-no-reboot \
	-d int,cpu_reset -D ./misc/qemu.log \
	-accel tcg
clean:
	rm -rf build/


