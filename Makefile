NC     = \033[0m
YELLOW = \033[1;33m
GREEN  = \033[1;32m

CXX=clang
LD=ld.lld
AS=nasm

CXXFLAGS=$(shell clang++ -v -E -x c++ -stdlib=libc++ /dev/null 2>&1 | grep -e "^ /" | grep -v gnu | sed -e "s/ \//-I\//g" | tr "\n" " ")
CXXFLAGS+=-O2 -Wall -Isrc -g -std=c++20
CXXFLAGS+=-ffreestanding -fno-stack-protector -fno-pic -mabi=sysv -mno-80387 -mno-mmx -mgeneral-regs-only
CXXFLAGS+=-mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -fno-exceptions -fno-rtti
LDFLAGS=-z max-page-size=0x1000 -T kernel.ld -nostdlib -static
ASFLAGS=-f elf64

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d))

KERNEL_CXX_OBJ=$(patsubst src/%.cpp, build/kernel/%.o, $(call rwildcard, src, *.cpp))
KERNEL_ASM_OBJ=$(patsubst src/%.asm, build/kernel/asm/%.o, $(call rwildcard, src, *.asm))
KERNEL_HEADERS=$(call rwildcard, src, *.hpp)

all: build/qos install

build/kernel/%.o: src/%.cpp $(KERNEL_HEADERS)
	@ mkdir -p $(@D)
	@ echo -e "$(GREEN)COMPILING CXX OBJECT$(NC) $@"
	@ $(CXX) $(CXXFLAGS) -c $< -o $@

build/kernel/asm/%.o: src/%.asm
	@ mkdir -p $(@D)
	@ echo -e "$(GREEN)COMPILING ASM OBJECT$(NC) $@"
	@ $(AS) $(ASFLAGS) $< -o $@

build/qos: $(KERNEL_CXX_OBJ) $(KERNEL_ASM_OBJ)
	@ echo -e "$(GREEN)LINKING KERNEL$(NC) $@"
	@ $(LD) $(LDFLAGS) $^ -o $@

run: all
	@ qemu-system-x86_64 -boot d -cdrom build/image.iso -m 2G -machine q35 -cpu qemu64 -smp 2

install: build/qos
	@ if [ -d "limine" ]; then \
		cd limine; git pull; cd ..; \
	  else \
		git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth=1; \
	  fi
	@ $(MAKE) -C limine
	@ mkdir -p build/iso_root
	@ cp build/qos fonts/Uni2-VGA16.psf limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin build/iso_root
	@ xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine-eltorito-efi.bin \
		--efi-boot-part --efi-boot-image --protective-msdos-label build/iso_root -o build/image.iso -quiet &> /dev/null
	@ ./limine/limine-install build/image.iso &> /dev/null

clean:
	@ echo -e "$(YELLOW)CLEANING PROJECT$(NC)"
	@ rm -rf build/kernel build/iso_root
