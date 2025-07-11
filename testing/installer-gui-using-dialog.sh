#!/bin/ash

# Alpine Linux UEFI-only BTRFS Installation with Desktop Environment
set -e

# Install dialog silently if missing
if ! command -v dialog >/dev/null; then
    apk add dialog >/dev/null 2>&1
fi

# Your original ASCII art and colors - PRESERVED EXACTLY
RED='\033[38;2;255;0;0m'
CYAN='\033[38;2;0;255;255m'
NC='\033[0m'

show_ascii() {
    clear
    echo -e "${RED}░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
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

# Your original perform_installation function - MODIFIED FOR DESKTOP SELECTION
perform_installation() {
    show_ascii

    # Check if running as root
    if [ "$(id -u)" -ne 0 ]; then
        dialog --msgbox "This script must be run as root or with sudo" 8 40
        exit 1
    fi

    # Verify UEFI
    if [ ! -d /sys/firmware/efi ]; then
        dialog --msgbox "ERROR: This script requires UEFI boot mode" 8 40
        exit 1
    fi

    # Show summary and confirm
    dialog --yesno "About to install to $TARGET_DISK with these settings:\n\nHostname: $HOSTNAME\nTimezone: $TIMEZONE\nKeymap: $KEYMAP\nUsername: $USER_NAME\nDesktop: $DESKTOP_ENV\n\nContinue?" 15 60
    if [ $? -ne 0 ]; then
        dialog --msgbox "Installation cancelled." 6 40
        exit 1
    fi

    # Installation steps
    dialog --infobox "Installing required packages..." 6 40
    cyan_output apk add btrfs-progs parted dosfstools grub-efi efibootmgr
    cyan_output modprobe btrfs

    dialog --infobox "Partitioning disk..." 6 40
    cyan_output parted -s "$TARGET_DISK" mklabel gpt
    cyan_output parted -s "$TARGET_DISK" mkpart primary 1MiB 513MiB
    cyan_output parted -s "$TARGET_DISK" set 1 esp on
    cyan_output parted -s "$TARGET_DISK" mkpart primary 513MiB 100%

    dialog --infobox "Creating filesystems..." 6 40
    cyan_output mkfs.vfat -F32 "${TARGET_DISK}1"
    cyan_output mkfs.btrfs -f "${TARGET_DISK}2"

    dialog --infobox "Creating BTRFS subvolumes..." 6 40
    cyan_output mount "${TARGET_DISK}2" /mnt
    cyan_output btrfs subvolume create /mnt/@
    cyan_output btrfs subvolume create /mnt/@home
    cyan_output btrfs subvolume create /mnt/@root
    cyan_output btrfs subvolume create /mnt/@srv
    cyan_output btrfs subvolume create /mnt/@tmp
    cyan_output btrfs subvolume create /mnt/@log
    cyan_output btrfs subvolume create /mnt/@cache
    cyan_output umount /mnt

    dialog --infobox "Mounting filesystems..." 6 40
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

    dialog --infobox "Installing base system..." 6 40
    cyan_output setup-disk -m sys /mnt

    dialog --infobox "Setting up chroot environment..." 6 40
    cyan_output mount -t proc none /mnt/proc
    cyan_output mount --rbind /dev /mnt/dev
    cyan_output mount --rbind /sys /mnt/sys

    # Determine login manager based on desktop environment
    case $DESKTOP_ENV in
        "KDE Plasma") LOGIN_MANAGER="sddm" ;;
        "GNOME") LOGIN_MANAGER="gdm" ;;
        "XFCE"|"MATE"|"LXQt") LOGIN_MANAGER="lightdm" ;;
        *) LOGIN_MANAGER="none" ;;
    esac

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

# Use setup-desktop for the selected environment
case "$DESKTOP_ENV" in
    "KDE Plasma")
        setup-desktop plasma
        ;;
    "GNOME")
        setup-desktop gnome
        ;;
    "XFCE")
        setup-desktop xfce
        ;;
    "MATE")
        setup-desktop mate
        ;;
    "LXQt")
        setup-desktop lxqt
        ;;
    *)
        echo "No desktop environment selected"
        ;;
esac

echo -e "${CYAN}Installing bootloader...${NC}"
apk add grub-efi efibootmgr
grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE
grub-mkconfig -o /boot/grub/grub.cfg

echo -e "${CYAN}Enabling services...${NC}"
rc-update add dbus
rc-update add networkmanager

if [ "$LOGIN_MANAGER" != "none" ]; then
    # Ensure login manager is installed (setup-desktop should handle this)
    rc-update add $LOGIN_MANAGER
fi

rm /setup-chroot.sh
CHROOT

    dialog --infobox "Running chroot configuration..." 6 40
    chmod +x /mnt/setup-chroot.sh
    chroot /mnt /setup-chroot.sh

    dialog --msgbox "Installation complete!" 6 40

    # Post-install options
    while true; do
        choice=$(dialog --clear --title "Installation Complete" \
                       --menu "Choose an option:" 12 40 3 \
                       1 "Reboot now" \
                       2 "Chroot into installed system" \
                       3 "Exit without rebooting" 3>&1 1>&2 2>&3)

        case $choice in
            1)
                dialog --infobox "Rebooting..." 6 40
                reboot
                ;;
            2)
                dialog --infobox "Chrooting into installed system..." 6 40
                mount "${TARGET_DISK}1" /mnt/boot/efi
                mount -o subvol=@ "${TARGET_DISK}2" /mnt
                mount -t proc none /mnt/proc
                mount --rbind /dev /mnt/dev
                mount --rbind /sys /mnt/sys
                mount --rbind /dev/pts /mnt/dev/pts
                dialog --msgbox "Entering chroot. Type 'exit' when done." 6 40
                chroot /mnt /bin/ash
                dialog --infobox "Exiting chroot..." 6 40
                umount -l /mnt
                ;;
            3)
                dialog --msgbox "Exiting. You can reboot manually when ready." 6 40
                exit 0
                ;;
        esac
    done
}

# Dialog-based configuration
configure_installation() {
    # Disk selection
    TARGET_DISK=$(lsblk -d -n -l -o NAME | grep -E '^[a-z]+$' | awk '{print "/dev/" $1}')
    TARGET_DISK=$(dialog --title "Target Disk" --menu "Select target disk:" 15 40 4 $(echo $TARGET_DISK | awk '{print $1, " "}') 3>&1 1>&2 2>&3)

    # Hostname
    HOSTNAME=$(dialog --title "Hostname" --inputbox "Enter hostname:" 8 40 "alpine" 3>&1 1>&2 2>&3)

    # Timezone
    TIMEZONE=$(dialog --title "Timezone" --inputbox "Enter timezone (e.g. UTC):" 8 40 "UTC" 3>&1 1>&2 2>&3)

    # Keymap
    KEYMAP=$(dialog --title "Keymap" --inputbox "Enter keymap (e.g. us):" 8 40 "us" 3>&1 1>&2 2>&3)

    # User configuration
    USER_NAME=$(dialog --title "Username" --inputbox "Enter username:" 8 40 "user" 3>&1 1>&2 2>&3)
    USER_PASSWORD=$(dialog --title "User Password" --insecure --passwordbox "Enter user password:" 8 40 3>&1 1>&2 2>&3)
    ROOT_PASSWORD=$(dialog --title "Root Password" --insecure --passwordbox "Enter root password:" 8 40 3>&1 1>&2 2>&3)

    # Desktop environment selection
    DESKTOP_ENV=$(dialog --title "Desktop Environment" --menu "Select desktop environment:" 15 40 5 \
        "KDE Plasma" "KDE Plasma Desktop" \
        "GNOME" "GNOME Desktop" \
        "XFCE" "XFCE Desktop" \
        "MATE" "MATE Desktop" \
        "LXQt" "LXQt Desktop" 3>&1 1>&2 2>&3)
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
