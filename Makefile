CFILES    := $(shell find src/ -type f -name '*.cpp')
CC         = gcc
LD         = ld
OBJ := $(shell find src/ -type f -name '*.o')
KERNEL_HDD = build/disk.hdd
KERNEL_ELF = kernel.elf
ASMFILES := $(shell find src/ -type f -name '*.asm')

OBJFILES := $(patsubst %.cpp,%.o,$(CFILES))
ASMOBJFILES := $(patsubst %.asm,%.o,$(ASMFILES))
CHARDFLAGS := $(CFLAGS)               \
	-DBUILD_TIME='"$(BUILD_TIME)"' \
	-std=c++17                     \
	-g \
	-masm=intel                    \
	-fno-pic                       \
	-no-pie \
	-mno-sse                       \
	-m64 \
	-mno-sse2                      \
	-mno-mmx                       \
	-mno-80387                     \
	-mno-red-zone                  \
	-mcmodel=large                \
	-ffreestanding                 \
	-fno-stack-protector           \
	-fno-omit-frame-pointer        \
	-Isrc/                         \

LDHARDFLAGS := $(LDFLAGS)        \
	-nostdlib                 \
	-no-pie                   \
	-z max-page-size=0x1000   \
	-T src/linker.ld

.PHONY: clean
.DEFAULT_GOAL = $(KERNEL_HDD)

disk: $(KERNEL_HDD)
run: $(KERNEL_HDD)
	qemu-system-x86_64 -m 2G -s -device pvpanic -serial stdio -enable-kvm --no-reboot panic=-1 -d int -d guest_errors -hda $(KERNEL_HDD)
runvbox: $(KERNEL_HDD)
	@VBoxManage -q startvm --putenv VBOX_GUI_DBG_ENABLED=true wingOS64
	@nc localhost 1234
super:
	-rm -f $(KERNEL_HDD) $(KERNEL_ELF) $(OBJ)  $(OBJFILES) $(ASMOBJFILES)
	make

	@objdump kernel.elf -f -s -d --source > kernel.map
	make runvbox
%.o: %.cpp
	@echo "cpp [BUILD] $<"
	$(CC) $(CHARDFLAGS) -c $< -o $@
%.o: %.asm
	@echo "nasm [BUILD] $<"
	@nasm $< -o $@ -felf64 -F dwarf -g -w+all -Werror
$(KERNEL_ELF): $(OBJFILES) $(ASMOBJFILES)
	@ld $(LDHARDFLAGS) $(OBJFILES) $(ASMOBJFILES) -o $@

$(KERNEL_HDD): $(KERNEL_ELF)
	-mkdir build
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(KERNEL_HDD)
	parted -s $(KERNEL_HDD) mklabel msdos
	parted -s $(KERNEL_HDD) mkpart primary 1 100%
	echfs-utils -m -p0 $(KERNEL_HDD) quick-format 32768
	echfs-utils -m -p0 $(KERNEL_HDD) import $(KERNEL_ELF) $(KERNEL_ELF)
	echfs-utils -m -p0 $(KERNEL_HDD) import qloader2.cfg qloader2.cfg
	qloader2/qloader2-install qloader2/qloader2.bin $(KERNEL_HDD)

clean:
	-rm -f $(KERNEL_HDD) $(KERNEL_ELF) $(OBJ)
