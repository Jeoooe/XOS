BUILD:=../build
SRC:=.

MULTIBOOT2:=0x10000
# ENTRYPOINT:=$(shell python -c "print(f'0x{$(MULTIBOOT2) + 64:x}')")
ENTRYPOINT:=0x10040

CFLAGS:= -m32
CFLAGS+= -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib
CFLAGS+= -fno-stack-protector
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g
INCLUDE:=-I$(SRC)/include

$(BUILD)/boot/%.bin: $(SRC)/boot/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $(DEBUG) $< -o $@

$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

LDFLAGS:= -m elf_i386 \
		-static \
		-Ttext $(ENTRYPOINT) \
		--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})

$(BUILD)/kernel.bin: \
	$(BUILD)/kernel/start.o \
	$(BUILD)/kernel/main.o \
	$(BUILD)/kernel/io.o \
	$(BUILD)/kernel/device.o \
	$(BUILD)/kernel/console.o \
	$(BUILD)/kernel/printk.o \
	$(BUILD)/kernel/assert.o \
	$(BUILD)/kernel/schedule.o \
	$(BUILD)/kernel/global.o \
	$(BUILD)/kernel/task.o \
	$(BUILD)/kernel/interrupt.o \
	$(BUILD)/kernel/thread.o \
	$(BUILD)/kernel/mutex.o \
	$(BUILD)/kernel/gate.o \
	$(BUILD)/kernel/handler.o \
	$(BUILD)/kernel/debug.o \
	$(BUILD)/kernel/clock.o \
	$(BUILD)/kernel/time.o \
	$(BUILD)/kernel/rtc.o \
	$(BUILD)/kernel/ide.o \
	$(BUILD)/kernel/memory.o \
	$(BUILD)/kernel/arena.o \
	$(BUILD)/kernel/keyboard.o \
	$(BUILD)/lib/bitmap.o \
	$(BUILD)/lib/string.o \
	$(BUILD)/lib/vsprintf.o \
	$(BUILD)/lib/stdlib.o \
	$(BUILD)/lib/syscall.o \
	$(BUILD)/lib/list.o \
	$(BUILD)/lib/fifo.o \
	$(BUILD)/lib/printf.o \
	
	$(shell mkdir -p $(dir $@))
	ld ${LDFLAGS} $^ -o $@

$(BUILD)/system.bin: $(BUILD)/kernel.bin
	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
	$(BUILD)/boot/loader.bin \
	$(BUILD)/system.bin \
	$(BUILD)/system.map \
	$(SRC)/utils/master.sfdisk \
	
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
	
	test -n "$$(find $(BUILD)/system.bin -size -100k)"

	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

	sfdisk $@ < $(SRC)/utils/master.sfdisk

$(BUILD)/slave.img:
	yes | bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@

IMAGES:= $(BUILD)/master.img $(BUILD)/slave.img

image: $(IMAGES)


$(BUILD)/kernel.iso : $(BUILD)/kernel.bin $(SRC)/utils/grub.cfg
#文件合法
	grub-file --is-x86-multiboot2 $<

	mkdir -p $(BUILD)/iso/boot/grub

	cp $< $(BUILD)/iso/boot
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub

	grub-mkrescue -o $@ $(BUILD)/iso

.PHONY: bochs bochsb
bochs: $(IMAGES)
	bochs -q -f ../bochs/bochsrc -unlock

bochsb: $(BUILD)/kernel.iso
	bochs -q -f ../bochs/bochsrc.grub -unlock


QEMU:= qemu-system-i386 
QEMU+= -m 32M 
QEMU+= -audiodev pa,id=hda 
QEMU+= -machine pcspk-audiodev=hda 
QEMU+= -rtc base=localtime 
QEMU+= -drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw 
QEMU+= -drive file=$(BUILD)/slave.img,if=ide,index=1,media=disk,format=raw 

QEMU_DISK:=-boot c 
	

QEMU_CDROM:=-boot d \
	-drive file=$(BUILD)/kernel.iso,media=cdrom \

QEMU_DEBUG:=-s -S



.PHONY: qemu qemug qemub
qemu: $(IMAGES)
	$(QEMU) $(QEMU_DISK)

qemug: $(IMAGES)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

qemub: $(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM) 



.PHONY: clean
clean:
	rm -rf $(BUILD)/