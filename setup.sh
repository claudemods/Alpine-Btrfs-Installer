#!/bin/ash

# Alpine Linux BTRFS Installation Script
# Features:
# - BTRFS with zstd:22 compression (forced)
# - Detailed subvolume structure
# - No swap configuration
# - Optimized mount options

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
apk add btrfs-progs parted e2fsprogs

# Partition disk
echo "Partitioning $TARGET_DISK..."
parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB   # /boot
parted -s "$TARGET_DISK" set 1 boot on
parted -s "$TARGET_DISK" mkpart primary 513MiB 100%   # /

# Create filesystems
echo "Creating filesystems..."
mkfs.ext4 "${TARGET_DISK}1"
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
mkdir -p /mnt/{boot,home,root,srv,tmp,var/{cache,log},var/lib/{portables,machines}}
mount "${TARGET_DISK}1" /mnt/boot
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
${TARGET_DISK}1 /boot ext4 rw,relatime 0 2
${TARGET_DISK}2 / btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@ 0 1
${TARGET_DISK}2 /home btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@home 0 2
${TARGET_DISK}2 /root btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@root 0 2
${TARGET_DISK}2 /srv btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@srv 0 2
${TARGET_DISK}2 /tmp btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@tmp 0 2
${TARGET_DISK}2 /var/log btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@log 0 2
${TARGET_DISK}2 /var/cache btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@cache 0 2
EOF

# Additional setup
apk add sudo
echo '%wheel ALL=(ALL) ALL' > /etc/sudoers.d/wheel

# Cleanup
rm /setup-chroot.sh
CHROOT

# Execute chroot script
chmod +x /mnt/setup-chroot.sh
chroot /mnt /setup-chroot.sh

# Cleanup
umount -R /mnt
echo "Installation complete! Reboot into your new system."
