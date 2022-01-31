if [ -d "limine" ]
then
  cd limine && git pull && cd ..
else
  git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
fi

make -C limine

mkdir -p build/iso_root

cp build/qos limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin build/iso_root

xorriso -as mkisofs -b limine-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine-eltorito-efi.bin \
    --efi-boot-part --efi-boot-image --protective-msdos-label build/iso_root -o build/image.iso

./limine/limine-install build/image.iso