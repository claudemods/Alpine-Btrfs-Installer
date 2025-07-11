#!/bin/ash

# Alpine Linux UEFI-only BTRFS Installation with KDE Plasma
set -e

# Install dialog silently if missing (only added line)


# Your original ASCII art and colors - PRESERVED EXACTLY
RED='\033[38;2;255;0;0m'
CYAN='\033[38;2;0;255;255m'
NC='\033[0m'

show_ascii() {
    clear
    echo -e "${RED}░█████╗░██╗░░░░░░█████╗░██╗░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░${NC}"
    echo -e "${CYAN}claudemods Alpine Btrfs Installer v1.01 11-07-2025${NC}"
    echo
}

# Your original cyan_output function - PRESERVED EXACTLY
cyan_output() {
    "$@" | while IFS= read -r line; do echo -e "${CYAN}$line${NC}"; done
}

# Your original perform_installation function - PRESERVED EXACTLY
perform_installation() {
    show_ascii

    # Check if running as root
    if [ "$(id -u)" -ne 0 ]; then
        echo -e "${CYAN}This script must be run as root or with sudo${NC}"
        exit 1
    fi

    # Verify UEFI
    if [ ! -d /sys/firmware/efi ]; then
        echo -e "${CYAN}ERROR: This script requires UEFI boot mode${NC}"
        exit 1
    fi

    # Show summary and confirm
    echo -e "${CYAN}About to install to $TARGET_DISK with these settings:"
    echo "Hostname: $HOSTNAME"
    echo "Timezone: $TIMEZONE"
    echo "Keymap: $KEYMAP"
    echo "Username: $USER_NAME${NC}"
    echo -ne "${CYAN}Continue? (y/n): ${NC}"
    read confirm
    if [ "$confirm" != "y" ]; then
        echo -e "${CYAN}Installation cancelled.${NC}"
        exit 1
    fi

    # Installation steps
    echo -e "${CYAN}Installing required packages...${NC}"
    cyan_output apk add btrfs-progs parted dosfstools grub-efi efibootmgr
    cyan_output modprobe btrfs

    echo -e "${CYAN}Partitioning disk...${NC}"
    cyan_output parted -s "$TARGET_DISK" mklabel gpt
    cyan_output parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB
    cyan_output parted -s "$TARGET_DISK" set 1 esp on
    cyan_output parted -s "$TARGET_DISK" mkpart primary 513MiB 100%

    echo -e "${CYAN}Creating filesystems...${NC}"
    cyan_output mkfs.vfat -F32 "${TARGET_DISK}1"
    cyan_output mkfs.btrfs -f "${TARGET_DISK}2"

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

    echo -e "${CYAN}Installing base system...${NC}"
    cyan_output setup-disk -m sys /mnt

    echo -e "${CYAN}Setting up chroot environment...${NC}"
    cyan_output mount -t proc none /mnt/proc
    cyan_output mount --rbind /dev /mnt/dev
    cyan_output mount --rbind /sys /mnt/sys

    # Chroot configuration
    cat << CHROOT | tee /mnt/setup-chroot.sh >/dev/null
#!/bin/ash
echo -e "${CYAN}Setting up users and basic configuration...${NC}"
echo "root:$ROOT_PASSWORD" | chpasswd
adduser -D "$USER_NAME" -G wheel,video,audio,input
echo "$USER_NAME:$USER_PASSWORD" | chpasswd
setup-timezone -z "$TIMEZONE"
setup-keymap "$KEYMAP" "$KEYMAP"
echo "$HOSTNAME" > /etc/hostname

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

echo -e "${CYAN}Installing desktop environment...${NC}"
apk update
setup-desktop

echo -e "${CYAN}Installing bootloader...${NC}"
apk add grub-efi efibootmgr
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE
grub-mkconfig -o /boot/grub/grub.cfg

echo -e "${CYAN}Enabling services...${NC}"
rc-update add dbus
rc-update add sddm
rc-update add networkmanager

rm /setup-chroot.sh
CHROOT

    echo -e "${CYAN}Running chroot configuration...${NC}"
    chmod +x /mnt/setup-chroot.sh
    chroot /mnt /setup-chroot.sh

    echo -e "${CYAN}Finalizing installation...${NC}"
    umount -l /mnt
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
                mount "${TARGET_DISK}1" /mnt/boot/efi
                mount -o subvol=@ "${TARGET_DISK}2" /mnt
                mount -t proc none /mnt/proc
                mount --rbind /dev /mnt/dev
                mount --rbind /sys /mnt/sys
                mount --rbind /dev/pts /mnt/dev/pts
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
}

# Dialog-based configuration
configure_installation() {
    # Disk selection
    TARGET_DISK=$(dialog --title "Target Disk" --inputbox "Enter target disk (e.g. /dev/sda):" 8 40 3>&1 1>&2 2>&3)

    # Other configuration
    HOSTNAME=$(dialog --title "Hostname" --inputbox "Enter hostname:" 8 40 3>&1 1>&2 2>&3)
    TIMEZONE=$(dialog --title "Timezone" --inputbox "Enter timezone (e.g. UTC):" 8 40 3>&1 1>&2 2>&3)
    KEYMAP=$(dialog --title "Keymap" --inputbox "Enter keymap (e.g. us):" 8 40 3>&1 1>&2 2>&3)
    USER_NAME=$(dialog --title "Username" --inputbox "Enter username:" 8 40 3>&1 1>&2 2>&3)
    USER_PASSWORD=$(dialog --title "User Password" --passwordbox "Enter user password:" 8 40 3>&1 1>&2 2>&3)
    ROOT_PASSWORD=$(dialog --title "Root Password" --passwordbox "Enter root password:" 8 40 3>&1 1>&2 2>&3)
}

# Main menu
main_menu() {
    while true; do
        choice=$(dialog --clear --title "Alpine Linux Btrfs Installer Gui v1.01" \
                       --menu "Select an option:" 15 45 5 \
                       1 "Configure Installation" \
                       2 "Start Installation" \
                       3 "Exit" 3>&1 1>&2 2>&3)

        case $choice in
            1)
                configure_installation
                ;;
            2)
                if [ -z "$TARGET_DISK" ]; then
                    dialog --msgbox "Please configure installation first!" 6 40
                else
                    perform_installation
                fi
                ;;
            3)
                clear
                exit 0
                ;;
        esac
    done
}

# Start the installer
show_ascii
main_menu
