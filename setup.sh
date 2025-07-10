#!/bin/ash

# Alpine Linux UEFI-only BTRFS Installation with KDE Plasma
set -e

# Configuration
TARGET_DISK="/dev/sda"
HOSTNAME="alpine-plasma"
TIMEZONE="UTC"
KEYMAP="us"
ROOT_PASSWORD="alpine"
USER_NAME="user"
USER_PASSWORD="user"
PLASMA_PKGS="plasma-desktop plasma-nm sddm konsole dolphin kate plasma-pa plasma-systemmonitor kdegraphics-thumbnailers ark gwenview"

# Verify UEFI
if [ ! -d /sys/firmware/efi ]; then
    echo "ERROR: This script requires UEFI boot mode"
    exit 1
fi

# Install prerequisites
apk add btrfs-progs parted dosfstools grub-efi efibootmgr

# Partitioning (GPT with EFI system partition)
parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB
parted -s "$TARGET_DISK" set 1 esp on
parted -s "$TARGET_DISK" mkpart primary 513MiB 100%

# Filesystems
mkfs.vfat -F32 "${TARGET_DISK}1"
mkfs.btrfs -f "${TARGET_DISK}2"

# BTRFS subvolumes
mount "${TARGET_DISK}2" /mnt
btrfs subvolume create /mnt/@
btrfs subvolume create /mnt/@home
btrfs subvolume create /mnt/@root
btrfs subvolume create /mnt/@srv
btrfs subvolume create /mnt/@tmp
btrfs subvolume create /mnt/@log
btrfs subvolume create /mnt/@cache
umount /mnt

# Mount with compression
mount -o subvol=@,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt
mkdir -p /mnt/boot/efi
mount "${TARGET_DISK}1" /mnt/boot/efi
mkdir -p /mnt/{home,root,srv,tmp,var/{cache,log}}

mount -o subvol=@home,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/home
mount -o subvol=@root,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/root
mount -o subvol=@srv,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/srv
mount -o subvol=@tmp,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/tmp
mount -o subvol=@log,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/log
mount -o subvol=@cache,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/cache

# Base system
setup-disk -m sys /mnt

# Chroot setup
mount -t proc none /mnt/proc
mount --rbind /dev /mnt/dev
mount --rbind /sys /mnt/sys

# Chroot configuration
cat << CHROOT > /mnt/setup-chroot.sh
#!/bin/ash

# System basics
echo "root:$ROOT_PASSWORD" | chpasswd
adduser -D "$USER_NAME" -G wheel,video,audio,input
echo "$USER_NAME:$USER_PASSWORD" | chpasswd

setup-timezone -z "$TIMEZONE"
setup-keymap "$KEYMAP" "$KEYMAP"
echo "$HOSTNAME" > /etc/hostname

# Fstab
cat << EOF > /etc/fstab
${TARGET_DISK}1 /boot/efi vfat defaults 0 2
${TARGET_DISK}2 / btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@ 0 1
${TARGET_DISK}2 /home btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@home 0 2
${TARGET_DISK}2 /root btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@root 0 2
${TARGET_DISK}2 /srv btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@srv 0 2
${TARGET_DISK}2 /tmp btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@tmp 0 2
${TARGET_DISK}2 /var/log btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@log 0 2
${TARGET_DISK}2 /var/cache btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@cache 0 2
EOF

# KDE Plasma
sed -i 's/^#.*@community/&/' /etc/apk/repositories
apk update
apk add $PLASMA_PKGS xdg-desktop-portal xdg-desktop-portal-kde pipewire wireplumber

# Bootloader
apk add grub-efi efibootmgr
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE
grub-mkconfig -o /boot/grub/grub.cfg

# Enable services
rc-update add dbus
rc-update add sddm
rc-update add NetworkManager

# Cleanup
rm /setup-chroot.sh
CHROOT

# Execute configuration
chmod +x /mnt/setup-chroot.sh
chroot /mnt /setup-chroot.sh

# Finalize
umount -R /mnt
echo "Installation complete! Reboot into your KDE Plasma system."
echo "Note: UEFI boot mode required. Select 'ALPINE' in boot menu."
