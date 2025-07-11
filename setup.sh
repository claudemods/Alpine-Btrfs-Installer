#!/bin/ash

# Alpine Linux UEFI-only BTRFS Installation with KDE Plasma
set -e

# Color codes
RED='\033[38;2;255;0;0m'
CYAN='\033[38;2;0;255;255m'
NC='\033[0m' # No Color

# Function to display command outputs in cyan
cyan_output() {
    "$@" | while IFS= read -r line; do echo -e "${CYAN}$line${NC}"; done
}

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
    echo -e "${CYAN}This script must be run as root or with sudo${NC}"
    exit 1
fi

# Enable main and community repositories
echo -e "${CYAN}Enabling main and community repositories...${NC}"
cyan_output sed -i 's/^#.*\/v[0-9]\.[0-9]\/community/&\n/' /etc/apk/repositories
cyan_output apk update

# Ask for configuration
echo -ne "${CYAN}Enter target disk (e.g. /dev/sda): ${NC}"
read TARGET_DISK
echo -ne "${CYAN}Enter hostname: ${NC}"
read HOSTNAME
echo -ne "${CYAN}Enter timezone (e.g. UTC): ${NC}"
read TIMEZONE
echo -ne "${CYAN}Enter keymap (e.g. us): ${NC}"
read KEYMAP
echo -ne "${CYAN}Enter username: ${NC}"
read USER_NAME
echo -ne "${CYAN}Enter user password: ${NC}"
read -s USER_PASSWORD
echo
echo -ne "${CYAN}Enter root password: ${NC}"
read -s ROOT_PASSWORD
echo

# Kernel selection (only from main and community repos)
echo -e "${CYAN}"
echo "Select kernel to install:"
echo "1) linux-lts - Long-term support kernel, configured for a generous selection of hardware"
echo "2) linux-virt - Long-term support kernel, configured for VM guests"
echo "3) linux-stable - Stable kernel, configured for a generous selection of hardware (community)"
echo "4) linux-openpax - Kernel with OpenPAX patches for memory safety (community)"
echo -ne "Enter choice (1-4, default 1): ${NC}"
read KERNEL_CHOICE

case $KERNEL_CHOICE in
    1) KERNEL_PKG="linux-lts" ;;
    2) KERNEL_PKG="linux-virt" ;;
    3) KERNEL_PKG="linux-stable" ;;
    4) KERNEL_PKG="linux-openpax" ;;
    *) KERNEL_PKG="linux-lts" ;;
esac

# Verify UEFI
if [ ! -d /sys/firmware/efi ]; then
    echo -e "${CYAN}ERROR: This script requires UEFI boot mode${NC}"
    exit 1
fi

# Confirm before proceeding
echo
echo -e "${CYAN}About to install to $TARGET_DISK with these settings:"
echo "Hostname: $HOSTNAME"
echo "Timezone: $TIMEZONE"
echo "Keymap: $KEYMAP"
echo "Username: $USER_NAME"
echo "Kernel: $KERNEL_PKG${NC}"
echo -ne "${CYAN}Continue? (y/n): ${NC}"
read confirm
if [ "$confirm" != "y" ]; then
    echo -e "${CYAN}Installation cancelled.${NC}"
    exit 1
fi

# Install prerequisites (including parted)
echo -e "${CYAN}Installing required packages...${NC}"
cyan_output apk add btrfs-progs parted dosfstools grub-efi efibootmgr $KERNEL_PKG
cyan_output modprobe btrfs

# Partitioning (GPT with EFI system partition)
echo -e "${CYAN}Partitioning disk...${NC}"
cyan_output parted -s "$TARGET_DISK" mklabel gpt
cyan_output parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB
cyan_output parted -s "$TARGET_DISK" set 1 esp on
cyan_output parted -s "$TARGET_DISK" mkpart primary 513MiB 100%

# Filesystems
echo -e "${CYAN}Creating filesystems...${NC}"
cyan_output mkfs.vfat -F32 "${TARGET_DISK}1"
cyan_output mkfs.btrfs -f "${TARGET_DISK}2"

# BTRFS subvolumes
echo -e "${CYAN}Creating BTRFS subvolumes...${NC}"
cyan_output mount "${TARGET_DISK}2" /mnt
cyan_output btrfs subvolume create /mnt/@
cyan_output btrfs subvolume create /mnt/@home
cyan_output btrfs subvolume create /mnt/@root
cyan_output btrfs subvolume create /mnt/@srv
cyan_output btrfs subvolume create /mnt/@tmp
cyan_output btrfs subvolume create /mnt/@log
cyan_output btrfs subvolume create /mnt/@cache
cyan_output umount /mnt

# Mount with compression
echo -e "${CYAN}Mounting filesystems...${NC}"
cyan_output mount -o subvol=@,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt
cyan_output mkdir -p /mnt/boot/efi
cyan_output mount "${TARGET_DISK}1" /mnt/boot/efi

cyan_output mkdir -p /mnt/home
cyan_output mkdir -p /mnt/root
cyan_output mkdir -p /mnt/srv
cyan_output mkdir -p /mnt/tmp
cyan_output mkdir -p /mnt/var/cache
cyan_output mkdir -p /mnt/var/log

cyan_output mount -o subvol=@home,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/home
cyan_output mount -o subvol=@root,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/root
cyan_output mount -o subvol=@srv,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/srv
cyan_output mount -o subvol=@tmp,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/tmp
cyan_output mount -o subvol=@log,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/log
cyan_output mount -o subvol=@cache,compress=zstd:22,compress-force=zstd:22 "${TARGET_DISK}2" /mnt/var/cache

# Base system
echo -e "${CYAN}Installing base system...${NC}"
cyan_output setup-disk -m sys /mnt

# Chroot setup
echo -e "${CYAN}Setting up chroot environment...${NC}"
cyan_output mount -t proc none /mnt/proc
cyan_output mount --rbind /dev /mnt/dev
cyan_output mount --rbind /sys /mnt/sys

# Chroot configuration
cat << CHROOT | tee /mnt/setup-chroot.sh >/dev/null
#!/bin/ash

# Enable main and community repositories in chroot
echo -e "${CYAN}Enabling main and community repositories in chroot...${NC}"
sed -i 's/^#.*\/v[0-9]\.[0-9]\/community/&\n/' /etc/apk/repositories
apk update

# System basics
echo -e "${CYAN}Setting up users and basic configuration...${NC}"
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

# Install selected kernel in chroot
echo -e "${CYAN}Installing $KERNEL_PKG in chroot...${NC}"
apk add $KERNEL_PKG

# KDE Plasma
echo -e "${CYAN}Installing KDE Plasma...${NC}"
sed -i 's/^#.*@community/&/' /etc/apk/repositories
apk update
setup-desktop

# Bootloader
echo -e "${CYAN}Installing bootloader...${NC}"
apk add grub-efi efibootmgr parted
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE
grub-mkconfig -o /boot/grub/grub.cfg

# Enable services
echo -e "${CYAN}Enabling services...${NC}"
rc-update add dbus
rc-update add sddm
rc-update add networkmanager

# Cleanup
rm /setup-chroot.sh
CHROOT

# Execute configuration
echo -e "${CYAN}Running chroot configuration...${NC}"
chmod +x /mnt/setup-chroot.sh
cyan_output chroot /mnt /setup-chroot.sh

# Finalize
echo -e "${CYAN}Finalizing installation...${NC}"
cyan_output umount -R /mnt
echo -e "${CYAN}Installation complete!${NC}"

# Post-install options
while true; do
    echo -e "${CYAN}"
    echo "Choose an option:"
    echo "1) Reboot now"
    echo "2) Chroot into installed system"
    echo "3) Exit without rebooting"
    echo -ne "Enter your choice (1-3): ${NC}"
    read choice
    
    case $choice in
        1)
            echo -e "${CYAN}Rebooting...${NC}"
            reboot
            ;;
        2)
            echo -e "${CYAN}Chrooting into installed system...${NC}"
            sudo mount "${DRIVE_PATH}1" /mnt/boot/efi
            sudo mount -o subvol=@ "${DRIVE_PATH}2" /mnt
            mount -t proc none /mnt/proc
            mount --rbind /dev /mnt/dev
            mount --rbind /sys /mnt/sys
            echo -e "${CYAN}Entering chroot. Type 'exit' when done.${NC}"
            chroot /mnt /bin/ash
            echo -e "${CYAN}Exiting chroot...${NC}"
            umount -l /mnt
            ;;
        3)
            echo -e "${CYAN}Exiting. You can reboot manually when ready.${NC}"
            exit 0
            ;;
        *)
            echo -e "${CYAN}Invalid option. Please try again.${NC}"
            ;;
    esac
done
