NC     = \033[0m
YELLOW = \033[1;33m
GREEN  = \033[1;32m

CC = clang
LD = ld.lld
AS = nasm

CFLAGS = -O2 -Wall -Isrc -g -std=c2x -Ithirdparty
CFLAGS += -ffreestanding -fno-stack-protector -fpic -mabi=sysv -mno-80387 -mno-mmx -mgeneral-regs-only
CFLAGS += -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel
LDFLAGS = -z max-page-size=0x1000 -T kernel.ld -nostdlib -static
ASFLAGS = -f elf64

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d))

KERNEL_C_OBJ=$(patsubst src/%.c, build/kernel/%.o, $(call rwildcard, src, *.c))
KERNEL_ASM_OBJ=$(patsubst src/%.asm, build/kernel/asm/%.o, $(call rwildcard, src, *.asm))
KERNEL_HEADERS=$(call rwildcard, src, *.h)

all: build/qos install

build/kernel/%.o: src/%.c $(KERNEL_HEADERS)
	@ mkdir -p $(@D)
	@ echo -e "$(GREEN)COMPILING FILE$(NC) $@"
	@ $(CC) $(CFLAGS) -c $< -o $@

build/kernel/asm/%.o: src/%.asm
	@ mkdir -p $(@D)
	@ echo -e "$(GREEN)COMPILING FILE$(NC) $@"
	@ $(AS) $(ASFLAGS) $< -o $@

build/qos: $(KERNEL_C_OBJ) $(KERNEL_ASM_OBJ)
	@ echo -e "$(GREEN)LINKING KERNEL$(NC) $@"
	@ $(LD) $(LDFLAGS) $^ -o $@

run: all
	@ qemu-system-x86_64 -boot d -cdrom build/image.iso -m 2G -machine q35 -cpu qemu64 -smp 2 -enable-kvm

install: build/qos
	@ if [ ! -d "build/limine" ]; then \
		git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth=1 build/limine; \
		$(MAKE) -C build/limine; \
		mkdir -p build/iso_root; \
		cp build/limine/limine.sys build/limine/limine-cd.bin build/limine/limine-cd-efi.bin build/iso_root; \
	  fi
	@ mkdir -p build/iso_root
	@ cp build/qos fonts/Tamsyn8x16r.psf limine.cfg build/iso_root
	@ xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine-cd-efi.bin \
		--efi-boot-part --efi-boot-image --protective-msdos-label build/iso_root -o build/image.iso -quiet &> /dev/null
	@ if [ ! -f "build/limine/done" ]; then \
 		./build/limine/limine-deploy build/image.iso &> /dev/null; \
 		touch build/limine/done; \
 	  fi

clean:
	@ echo -e "$(YELLOW)CLEANING PROJECT$(NC)"
	@ rm -rf build/kernel
