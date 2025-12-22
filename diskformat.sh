rm fat32.img

dd if=/dev/zero of=fat32.img bs=1M count=64

mkfs.fat -F 32 fat32.img