#!/bin/ash

# Alpine Linux BTRFS Installation Script with KDE Plasma (No Swap)
# Features:
# - BTRFS root filesystem with Zstd compression level 22
# - KDE Plasma desktop environment
# - No swap file/partition
# - Proper subvolume layout for snapshots
# - Optimized mount options

# Exit on error
set -e

# Configuration variables
TARGET_DISK="/dev/sda"       # Change this to your target disk
HOSTNAME="alpine-kde"
TIMEZONE="UTC"               # e.g. "America/New_York"
KEYMAP="us"                  # Keyboard layout
ROOT_PASSWORD="alpine"       # Change this!
USER_NAME="user"             # Regular user to create
USER_PASSWORD="user"         # Change this!
COMPRESS_LEVEL="22"          # Zstd compression level (1-22)

# Check if we're root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root"
    exit 1
fi

# Install required packages
echo "Installing required packages..."
apk add btrfs-progs parted e2fsprogs

# Partition the disk
echo "Partitioning $TARGET_DISK..."
parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB   # /boot (512MB)
parted -s "$TARGET_DISK" set 1 boot on
parted -s "$TARGET_DISK" mkpart primary 513MiB 100%   # / (remainder)

# Create filesystems
echo "Creating filesystems..."
mkfs.ext4 "${TARGET_DISK}1"
mkfs.btrfs -f "${TARGET_DISK}2"

# Mount the filesystems
echo "Mounting filesystems..."
mount "${TARGET_DISK}2" /mnt
mkdir /mnt/boot
mount "${TARGET_DISK}1" /mnt/boot

# Create BTRFS subvolumes
echo "Creating BTRFS subvolumes..."
btrfs subvolume create /mnt/@
btrfs subvolume create /mnt/@home
btrfs subvolume create /mnt/@snapshots
btrfs subvolume create /mnt/@var
btrfs subvolume create /mnt/@tmp
btrfs subvolume create /mnt/@opt
btrfs subvolume create /mnt/@srv

# Unmount to remount with subvolumes
umount /mnt

# Mount with subvolumes and compression
echo "Remounting with subvolumes and zstd:$COMPRESS_LEVEL compression..."
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@ "${TARGET_DISK}2" /mnt
mkdir -p /mnt/{boot,home,var,tmp,opt,srv,.snapshots}
mount "${TARGET_DISK}1" /mnt/boot
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@home "${TARGET_DISK}2" /mnt/home
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@var "${TARGET_DISK}2" /mnt/var
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@tmp "${TARGET_DISK}2" /mnt/tmp
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@opt "${TARGET_DISK}2" /mnt/opt
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@srv "${TARGET_DISK}2" /mnt/srv
mount -o noatime,compress=zstd:$COMPRESS_LEVEL,subvol=@snapshots "${TARGET_DISK}2" /mnt/.snapshots

# Install Alpine Linux base system
echo "Installing Alpine Linux..."
setup-disk -m sys /mnt

# Chroot into the new system
echo "Setting up system configuration..."
mount -t proc none /mnt/proc
mount --rbind /dev /mnt/dev
mount --rbind /sys /mnt/sys

echo "Creating BTRFS control device..."
mknod /mnt/dev/btrfs-control c 10 234
chmod 600 /mnt/dev/btrfs-control

# Create chroot script
cat << CHROOT > /mnt/setup-chroot.sh
#!/bin/ash

echo "Configuring persistent BTRFS control device..."
echo "btrfs-control" >> /etc/mdev.conf

# Set root password
echo "Setting root password..."
echo "root:$ROOT_PASSWORD" | chpasswd

# Create user
echo "Creating user $USER_NAME..."
adduser -D "$USER_NAME"
echo "$USER_NAME:$USER_PASSWORD" | chpasswd
addgroup "$USER_NAME" wheel
addgroup "$USER_NAME" video
addgroup "$USER_NAME" audio
addgroup "$USER_NAME" input
addgroup "$USER_NAME" plugdev

# Configure timezone
echo "Setting timezone to $TIMEZONE..."
setup-timezone -z "$TIMEZONE"

# Configure keymap
echo "Setting keymap to $KEYMAP..."
setup-keymap "$KEYMAP" "$KEYMAP"

# Set hostname
echo "Setting hostname to $HOSTNAME..."
echo "$HOSTNAME" > /etc/hostname

# Enable repositories
echo "Enabling repositories..."
sed -i 's/^#\s*@community/\@community/' /etc/apk/repositories
sed -i 's/^#\s*@testing/\@testing/' /etc/apk/repositories

# Update packages
echo "Updating packages..."
apk update
apk upgrade

# Install KDE Plasma
echo "Installing KDE Plasma..."
apk add plasma-desktop plasma-nm sddm konsole dolphin kate \
    plasma-pa plasma-systemmonitor plasma-thunderbolt plasma-firewall \
    plasma-browser-integration plasma-disks plasma-vault \
    kdegraphics-thumbnailers kdesdk-thumbnailers \
    ark gwenview okular spectacle print-manager \
    kdeconnect networkmanager-qt \
    ttf-dejavu ttf-liberation noto-fonts \
    pipewire wireplumber pipewire-pulse \
    xdg-desktop-portal xdg-desktop-portal-kde \
    flatpak

# Install additional useful packages
apk add sudo doas btrfs-progs snapper grub-btrfs \
    firefox chromium libreoffice \
    ffmpegthumbnailer \
    flatpak xdg-utils \
    elisa kaffeine \
    kcolorchooser kfind kgpg kmag kmousetool kmouth

# Configure sudo
echo "Configuring sudo..."
echo '%wheel ALL=(ALL) ALL' > /etc/sudoers.d/wheel

# Configure SDDM (KDE display manager)
echo "Configuring SDDM..."
rc-update add dbus
rc-update add sddm
rc-update add NetworkManager
rc-update add bluetooth

# Configure fstab with compression options
echo "Configuring fstab..."
cat << EOF > /etc/fstab
${TARGET_DISK}1 /boot ext4 rw,relatime 0 2
${TARGET_DISK}2 / btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@ 0 1
${TARGET_DISK}2 /home btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@home 0 2
${TARGET_DISK}2 /var btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@var 0 2
${TARGET_DISK}2 /tmp btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@tmp 0 2
${TARGET_DISK}2 /opt btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@opt 0 2
${TARGET_DISK}2 /srv btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@srv 0 2
${TARGET_DISK}2 /.snapshots btrfs rw,noatime,compress=zstd:${COMPRESS_LEVEL},subvol=@snapshots 0 2
EOF

# Enable btrfs compression by default
echo "Configuring mkinitfs for btrfs..."
echo "features=\"base btrfs ext4 kms mmc nvme raid scsi usb virtio\"" > /etc/mkinitfs/mkinitfs.conf
mkinitfs $(ls /lib/modules)

# Install GRUB
echo "Installing bootloader..."
grub-install "$TARGET_DISK"
grub-mkconfig -o /boot/grub/grub.cfg

# Configure snapper for snapshots
echo "Configuring snapper..."
apk add snapper
mkdir -p /.snapshots
btrfs subvolume delete /.snapshots
snapper --no-dbus -c root create-config /
snapper --no-dbus -c home create-config /home
sed -i 's/^TIMELINE_LIMIT_HOURLY=.*/TIMELINE_LIMIT_HOURLY="5"/' /etc/snapper/configs/root
sed -i 's/^TIMELINE_LIMIT_DAILY=.*/TIMELINE_LIMIT_DAILY="7"/' /etc/snapper/configs/root
sed -i 's/^TIMELINE_LIMIT_WEEKLY=.*/TIMELINE_LIMIT_WEEKLY="4"/' /etc/snapper/configs/root
sed -i 's/^TIMELINE_LIMIT_MONTHLY=.*/TIMELINE_LIMIT_MONTHLY="6"/' /etc/snapper/configs/root
sed -i 's/^TIMELINE_LIMIT_YEARLY=.*/TIMELINE_LIMIT_YEARLY="2"/' /etc/snapper/configs/root

# Enable services
echo "Enabling services..."
rc-update add devfs sysinit
rc-update add dmesg sysinit
rc-update add mdev sysinit
rc-update add hwclock boot
rc-update add modules boot
rc-update add sysctl boot
rc-update add hostname boot
rc-update add bootmisc boot
rc-update add syslog boot
rc-update add mount-ro shutdown
rc-update add killprocs shutdown
rc-update add savecache shutdown

# Clean up
rm /setup-chroot.sh
CHROOT

# Make chroot script executable and run it
chmod +x /mnt/setup-chroot.sh
chroot /mnt /setup-chroot.sh

# Clean up
echo "Cleaning up..."
umount -R /mnt

echo "Installation complete! You can now reboot into your new Alpine Linux KDE Plasma system with BTRFS (no swap)."
echo "Recommended next steps after first boot:"
echo "1. Configure KDE Plasma to your liking"
echo "2. Set up Flatpak if needed:"
echo "   flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo"
echo "3. Monitor system memory usage (since no swap is configured)"
echo "4. Install additional software as needed"
