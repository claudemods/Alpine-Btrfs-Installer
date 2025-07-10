#!/bin/ash

# Alpine Linux BTRFS Installation Script with FAT32 Boot
# Features:
# - FAT32 boot partition (for UEFI)
# - BTRFS root with zstd:22 compression (forced)
# - Guaranteed GRUB installation
# - Detailed subvolume structure

# Configuration
TARGET_DISK="/dev/sda"  # Change to your target disk
HOSTNAME="alpine-btrfs"
TIMEZONE="UTC"
KEYMAP="us"
ROOT_PASSWORD="alpine"  # Change this
USER_NAME="user"
USER_PASSWORD="user"    # Change this

# Check root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root"
    exit 1
fi

# Install required packages
echo "Installing required packages..."
apk add btrfs-progs parted dosfstools grub grub-efi efibootmgr

# Partition disk (FAT32 for boot)
echo "Partitioning $TARGET_DISK..."
parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB   # /boot (FAT32)
parted -s "$TARGET_DISK" set 1 esp on                 # EFI System Partition
parted -s "$TARGET_DISK" mkpart primary 513MiB 100%   # / (BTRFS)

# Create filesystems
echo "Creating filesystems..."
mkfs.vfat -F32 "${TARGET_DISK}1"                     # FAT32 for boot
mkfs.btrfs -f "${TARGET_DISK}2"

# Create BTRFS subvolumes
echo "Creating BTRFS subvolumes..."
mount "${TARGET_DISK}2" /mnt
btrfs subvolume create /mnt/@
btrfs subvolume create /mnt/@home
btrfs subvolume create /mnt/@root
btrfs subvolume create /mnt/@srv
btrfs subvolume create /mnt/@tmp
btrfs subvolume create /mnt/@log
btrfs subvolume create /mnt/@cache

# Mount with specified options
echo "Mounting with zstd:22 compression..."
mount -o subvol=@,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt
mkdir -p /mnt/boot/efi
mount "${TARGET_DISK}1" /mnt/boot/efi
mkdir -p /mnt/{home,root,srv,tmp,var/{cache,log},var/lib/{portables,machines}}
mount -o subvol=@home,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/home
mount -o subvol=@root,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/root
mount -o subvol=@srv,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/srv
mount -o subvol=@tmp,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/tmp
mount -o subvol=@log,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/log
mount -o subvol=@cache,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/cache

# Install base system
echo "Installing Alpine Linux..."
setup-disk -m sys /mnt

# Chroot setup
echo "Configuring system..."
mount -t proc none /mnt/proc
mount --rbind /dev /mnt/dev
mount --rbind /sys /mnt/sys

# Create chroot script
cat << CHROOT > /mnt/setup-chroot.sh
#!/bin/ash

# System configuration
echo "root:$ROOT_PASSWORD" | chpasswd
adduser -D "$USER_NAME"
echo "$USER_NAME:$USER_PASSWORD" | chpasswd
addgroup "$USER_NAME" wheel

setup-timezone -z "$TIMEZONE"
setup-keymap "$KEYMAP" "$KEYMAP"
echo "$HOSTNAME" > /etc/hostname

# Fstab configuration
cat << EOF > /etc/fstab
${TARGET_DISK}1 /boot/efi vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro 0 2
${TARGET_DISK}2 / btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@ 0 1
${TARGET_DISK}2 /home btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@home 0 2
${TARGET_DISK}2 /root btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@root 0 2
${TARGET_DISK}2 /srv btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@srv 0 2
${TARGET_DISK}2 /tmp btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@tmp 0 2
${TARGET_DISK}2 /var/log btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@log 0 2
${TARGET_DISK}2 /var/cache btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@cache 0 2
EOF

# Install GRUB properly
echo "Installing GRUB..."
apk add grub grub-efi efibootmgr
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=alpine --recheck
grub-mkconfig -o /boot/grub/grub.cfg

# Additional setup
apk add sudo
echo '%wheel ALL=(ALL) ALL' > /etc/sudoers.d/wheel

# Ensure kernel is properly configured
echo "Configuring kernel..."
apk add linux-lts
mkinitfs -b / $(ls /lib/modules)

# Cleanup
rm /setup-chroot.sh
CHROOT


# Cleanup
umount -R /mnt
echo "Installation complete! Important notes:"
echo "1. Ensure your BIOS is set to UEFI mode (not legacy/CSM)"
echo "2. The bootloader is installed at /boot/efi"
echo "3. Reboot and select Alpine Linux from your BIOS boot menu"
