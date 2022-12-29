#!/bin/bash

set -e

cargo build --release

if [ ! -d target/limine ]; then
	git clone https://github.com/limine-bootloader/limine --branch=v4.x-branch-binary --depth=1 target/limine
	make -C target/limine
	mkdir -p target/iso_root
	cp target/limine/limine.sys target/limine/limine-cd.bin target/limine/limine-cd-efi.bin target/iso_root
fi

if ! cmp --silent -- "target/x86_64-unknown-none/release/qos" "target/iso_root/qos"; then
	mkdir -p target/iso_root
	cp target/x86_64-unknown-none/release/qos \
	 fonts/Tamsyn8x16r.psf \
	 limine.cfg \
	 target/iso_root
	xorriso -as mkisofs \
		-b limine-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--efi-boot limine-cd-efi.bin \
		--efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		target/iso_root \
		-o target/image.iso \
		-quiet &> /dev/null
fi

if [ ! -f "target/limine/done" ]; then
	./target/limine/limine-deploy target/image.iso &> /dev/null
	touch target/limine/done
fi

QEMU_ARGS="-boot d -cdrom target/image.iso -m 2G -machine q35 -cpu qemu64 -smp 2 -d int \
            --no-reboot --no-shutdown"
DEBUG_ARGS="-s -S"

if [ $# -ne 0 ]; then
  qemu-system-x86_64 $QEMU_ARGS $DEBUG_ARGS
else
  qemu-system-x86_64 $QEMU_ARGS
fi