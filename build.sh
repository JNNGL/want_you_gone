#!/bin/bash
set -e

C_FLAGS="-c -O0 -std=gnu99 -ffreestanding -Wall -Isrc"
ASM_FLAGS="-felf"
LD_FLAGS="-T linker.ld -nostdlib"

C_FILES=$(find src -type f -name "*.c" | sed 's|^src/||')
ASM_FILES=$(find src -type f -name "*.asm" | sed 's|^src/||')

rm -rf build
mkdir build

objects=""

for dir in $(find src/* -type d | sed 's|^src/||'); do
  mkdir -p "build/$dir"
done

touch build/data.bin

resource_files=$(find resources -type f | sed 's|^resources/||')
for file in $resource_files; do
  printf "%s" "$file" >> build/data.bin
  printf '\x00' >> build/data.bin
  file_size=$(wc -c < "resources/$file")
  printf "0: %.8x" "$file_size" | sed -E 's/0: (..)(..)(..)(..)/0: \4\3\2\1/' | xxd -r -g0 >> build/data.bin
  cat "resources/$file" >> build/data.bin
done

printf "%s\x00" "DATAEND" >> build/data.bin

data_size=$(wc -c < build/data.bin)

printf "global data_file_size\ndata_file_size: dd %s\n" "$data_size" > build/constants.asm
printf "global boot_file_size\nboot_file_size: dd 524288 ; (not actual size)\n" >> build/constants.asm
printf "%%define bootloader_size 524288\n" >> build/constants.asm

for file in $C_FILES; do
  i686-elf-gcc $C_FLAGS "src/$file" -o "build/$file.o"
  objects="build/$file.o $objects"
done
for file in $ASM_FILES; do

  nasm $ASM_FLAGS "src/$file" -o "build/$file.o"
  objects="build/$file.o $objects"
done

i686-elf-gcc $LD_FLAGS $objects -o build/boot.bin

bootloader_size=$(wc -c < build/boot.bin)

printf "global data_file_size\ndata_file_size: dd %s\n" "$data_size" > build/constants.asm
printf "global boot_file_size\nboot_file_size: dd %s\n" "$bootloader_size" >> build/constants.asm
printf "%%define bootloader_size %s\n" "$bootloader_size" >> build/constants.asm

for file in $ASM_FILES; do
  nasm $ASM_FLAGS "src/$file" -o "build/$file.o"
done

i686-elf-gcc $LD_FLAGS $objects -o build/boot.bin

bootloader_size=$(wc -c < build/boot.bin)
remainder=$((bootloader_size % 512))
if [ "$remainder" -eq 0 ]; then
  padded_size=$bootloader_size
else
  padded_size=$((bootloader_size + 512 - remainder))
fi

if [ "$bootloader_size" -ne "$padded_size" ]; then
  truncate -s $padded_size build/boot.bin
fi

cat build/data.bin >> build/boot.bin