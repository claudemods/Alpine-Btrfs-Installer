#!/bin/ash

# Alpine Linux UEFI-only BTRFS Installation with KDE Plasma
set -e

# Color codes
RED='\033[38;2;255;0;0m'
CYAN='\033[38;2;0;255;255m'
NC='\033[0m' # No Color

# Display header
clear
echo -e "${RED}░█████╗░██╗░░░░░░█████╗░██╗░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░${NC}"
echo -e "${CYAN}claudemods Alpine Btrfs Installer v1.0 11-07-2025${NC}"
echo

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root or with "
    exit 1
fi

# Ask for configuration
read -p "Enter target disk (e.g. /dev/sda): " TARGET_DISK
read -p "Enter hostname: " HOSTNAME
read -p "Enter timezone (e.g. UTC): " TIMEZONE
read -p "Enter keymap (e.g. us): " KEYMAP
read -p "Enter username: " USER_NAME
read -s -p "Enter user password: " USER_PASSWORD
echo
read -s -p "Enter root password: " ROOT_PASSWORD
echo

# Verify UEFI
if [ ! -d /sys/firmware/efi ]; then
    echo "ERROR: This script requires UEFI boot mode"
    exit 1
fi

# Confirm before proceeding
echo
echo "About to install to $TARGET_DISK with these settings:"
echo "Hostname: $HOSTNAME"
echo "Timezone: $TIMEZONE"
echo "Keymap: $KEYMAP"
echo "Username: $USER_NAME"
read -p "Continue? (y/n): " confirm
if [ "$confirm" != "y" ]; then
    echo "Installation cancelled."
    exit 1
fi

# Install prerequisites
echo "Installing required packages..."
apk add btrfs-progs parted dosfstools grub-efi efibootmgr
modprobe btrfs

# Partitioning (GPT with EFI system partition)
echo "Partitioning disk..."
parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB
parted -s "$TARGET_DISK" set 1 esp on
parted -s "$TARGET_DISK" mkpart primary 513MiB 100%

# Filesystems
echo "Creating filesystems..."
mkfs.vfat -F32 "${TARGET_DISK}1"
mkfs.btrfs -f "${TARGET_DISK}2"

# BTRFS subvolumes
echo "Creating BTRFS subvolumes..."
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
echo "Mounting filesystems..."
mount -o subvol=@,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt
mkdir -p /mnt/boot/efi
mount "${TARGET_DISK}1" /mnt/boot/efi

mkdir -p /mnt/home
mkdir -p /mnt/root
mkdir -p /mnt/srv
mkdir -p /mnt/tmp
mkdir -p /mnt/var/cache
mkdir -p /mnt/var/log

mount -o subvol=@home,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/home
mount -o subvol=@root,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/root
mount -o subvol=@srv,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/srv
mount -o subvol=@tmp,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/tmp
mount -o subvol=@log,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/log
mount -o subvol=@cache,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/cache

# Base system
echo "Installing base system..."
setup-disk -m sys /mnt

# Chroot setup
mount -t proc none /mnt/proc
mount --rbind /dev /mnt/dev
mount --rbind /sys /mnt/sys

# Chroot configuration
cat << CHROOT |  tee /mnt/setup-chroot.sh >/dev/null
#!/bin/ash

# System basics
echo "Setting up users and basic configuration..."
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
echo "Installing KDE Plasma..."
sed -i 's/^#.*@community/&/' /etc/apk/repositories
apk update
setup-desktop

# Bootloader
echo "Installing bootloader..."
apk add grub-efi efibootmgr
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE
grub-mkconfig -o /boot/grub/grub.cfg

# Enable services
echo "Enabling services..."
rc-update add dbus
rc-update add sddm
rc-update add networkmanager

# Cleanup
rm /setup-chroot.sh
CHROOT

# Execute configuration
 chmod +x /mnt/setup-chroot.sh
chroot /mnt /setup-chroot.sh

# Finalize
umount -R /mnt
echo -e "${CYAN}Installation complete!${NC}"

# Ask for reboot
read -p "Reboot now? (y/n): " reboot_confirm
if [ "$reboot_confirm" = "y" ]; then
    echo "Rebooting..."
    reboot
else
    echo "You can reboot manually when ready."
fi
