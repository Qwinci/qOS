cmake_minimum_required(VERSION 3.21)
project(qOS)
include(toolchain.cmake)

#set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_C_STANDARD 23)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS src/*.c src/*.h src/*.asm)
file(GLOB_RECURSE LAI_SRC thirdparty/lai/*.c)

add_executable(qos ${SRC} ${LAI_SRC})
target_compile_options(qos PRIVATE
        $<$<COMPILE_LANGUAGE:C>: -Wall -fno-stack-protector -fPIC -mabi=sysv -mno-80387 -mno-mmx -mgeneral-regs-only
        -mno-3dnow -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel -fno-builtin-printf -masm=intel -gdwarf-4
        -fno-omit-frame-pointer -fno-builtin-free -Wno-fortify-source -Wno-interrupt-service-routine>)
target_include_directories(qos PRIVATE src thirdparty src/std thirdparty/lai/include)
target_link_options(qos PRIVATE -fPIE -z max-page-size=0x1000 -T ${PROJECT_SOURCE_DIR}/kernel.ld -nostdlib -static -static-pie)
add_custom_command(TARGET qos POST_BUILD
        COMMAND mkdir -p ${PROJECT_BINARY_DIR}/iso_root
        COMMAND cp qos ${PROJECT_SOURCE_DIR}/fonts/Tamsyn8x16r.psf ${PROJECT_SOURCE_DIR}/limine.cfg ${PROJECT_BINARY_DIR}/iso_root
        COMMAND xorriso
        -as mkisofs  -b limine-cd.bin -no-emul-boot
        -boot-load-size 4
        -boot-info-table
        --efi-boot
        limine-cd-efi.bin
        --efi-boot-part
        --efi-boot-image
        --protective-msdos-label
        ${PROJECT_BINARY_DIR}/iso_root
        -o
        ${PROJECT_BINARY_DIR}/image.iso
        -quiet &> /dev/null VERBATIM USES_TERMINAL)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/limine)
    execute_process(
            COMMAND git clone https://github.com/limine-bootloader/limine.git
                --branch=v4.x-branch-binary --depth=1 ${PROJECT_BINARY_DIR}/limine
    )
    execute_process(COMMAND make -C ${PROJECT_BINARY_DIR}/limine)
    execute_process(COMMAND mkdir -p ${PROJECT_BINARY_DIR}/iso_root)
    execute_process(
            COMMAND cp
            ${PROJECT_BINARY_DIR}/limine/limine.sys
            ${PROJECT_BINARY_DIR}/limine/limine-cd.bin
            ${PROJECT_BINARY_DIR}/limine/limine-cd-efi.bin
            ${PROJECT_BINARY_DIR}/iso_root)
endif()

add_custom_target(run_kvm
        COMMAND qemu-system-x86_64 -boot d -cdrom ${PROJECT_BINARY_DIR}/image.iso
        -m 2G -machine q35 -cpu host,invtsc -smp 2 -enable-kvm -d int -no-reboot -no-shutdown
        -device ich9-usb-uhci1 -device usb-kbd
        DEPENDS qos
        USES_TERMINAL VERBATIM)
add_custom_target(run
        COMMAND qemu-system-x86_64 -boot d -cdrom ${PROJECT_BINARY_DIR}/image.iso
        -m 4G -machine q35 -cpu qemu64 -smp 2 -d int -no-reboot -no-shutdown
        -device ich9-usb-uhci1 -device usb-kbd -device sb16
        DEPENDS qos
        USES_TERMINAL VERBATIM)
# qemu-xhci
add_custom_target(debug
        COMMAND qemu-system-x86_64 -boot d -cdrom ${PROJECT_BINARY_DIR}/image.iso
        -m 2G -machine q35 -cpu qemu64 -smp 2 -d int -no-reboot -no-shutdown -S -s
        DEPENDS qos
        USES_TERMINAL VERBATIM)
