#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <dialog.h>

namespace fs = std::filesystem;

// Constants
const std::string RED = "\033[38;2;255;0;0m";
const std::string CYAN = "\033[38;2;0;255;255m";
const std::string NC = "\033[0m";

// Global configuration
struct Config {
    std::string target_disk;
    std::string hostname;
    std::string timezone;
    std::string keymap;
    std::string user_name;
    std::string user_password;
    std::string root_password;
    std::string desktop_env;
    std::string bootloader;
    std::string init_system;
    std::string boot_filesystem;
    int compression_level;
};

Config config;

// Function prototypes
void show_ascii();
void configure_fastest_mirrors();
void perform_installation();
void configure_installation();
void main_menu();
void execute_command(const std::string& cmd, bool show_output = true);
std::string execute_command_with_output(const std::string& cmd);
std::vector<std::string> get_available_disks();
std::vector<std::string> get_timezones();
std::vector<std::string> get_keymaps();
bool is_efi_system();
bool is_root();

// Utility functions
void execute_command(const std::string& cmd, bool show_output) {
    if (show_output) {
        std::cout << CYAN << "Executing: " << cmd << NC << std::endl;
    }
    int status = system(cmd.c_str());
    if (status != 0 && show_output) {
        std::cerr << "Command failed with status: " << status << std::endl;
    }
}

std::string execute_command_with_output(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) {
        result += buffer.data();
    }
    return result;
}

std::vector<std::string> get_available_disks() {
    std::vector<std::string> disks;
    for (const auto& entry : fs::directory_iterator("/dev")) {
        std::string name = entry.path().filename();
        if (name.find("sd") == 0 || name.find("nvme") == 0 || name.find("vd") == 0) {
            disks.push_back("/dev/" + name);
        }
    }
    return disks;
}

bool is_efi_system() {
    return fs::exists("/sys/firmware/efi");
}

bool is_root() {
    return getuid() == 0;
}

void show_ascii() {
    system("clear");
    std::cout << RED << R"(
░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░)" << NC << std::endl;
    std::cout << CYAN << "Alpine Btrfs Installer v1.02 12-07-2025" << NC << std::endl << std::endl;
}

void configure_fastest_mirrors() {
    show_ascii();
    int choice = dialog_yesno("Fastest Mirrors", "Would you like to find and use the fastest mirrors?", 7, 50);
    
    if (choice == 0) {
        std::cout << CYAN << "Finding fastest mirrors..." << NC << std::endl;
        execute_command("apk add reflector >/dev/null 2>&1");
        execute_command("reflector --latest 20 --protocol https --sort rate --save /etc/apk/repositories");
        std::cout << CYAN << "Mirrorlist updated with fastest mirrors" << NC << std::endl;
    } else {
        std::cout << CYAN << "Using default mirrors" << NC << std::endl;
    }
}

void perform_installation() {
    show_ascii();

    if (!is_root()) {
        std::cerr << CYAN << "This script must be run as root or with sudo" << NC << std::endl;
        exit(1);
    }

    if (!is_efi_system()) {
        std::cerr << CYAN << "ERROR: This script requires UEFI boot mode" << NC << std::endl;
        exit(1);
    }

    std::cout << CYAN << "About to install to " << config.target_disk << " with these settings:" << NC << std::endl;
    std::cout << "Hostname: " << config.hostname << std::endl;
    std::cout << "Timezone: " << config.timezone << std::endl;
    std::cout << "Keymap: " << config.keymap << std::endl;
    std::cout << "Username: " << config.user_name << std::endl;
    std::cout << "Desktop: " << config.desktop_env << std::endl;
    std::cout << "Bootloader: " << config.bootloader << std::endl;
    std::cout << "Init System: " << config.init_system << std::endl;
    std::cout << "Boot Filesystem: " << config.boot_filesystem << std::endl;
    std::cout << "Compression Level: " << config.compression_level << NC << std::endl;
    
    std::cout << CYAN << "Continue? (y/n): " << NC;
    char confirm;
    std::cin >> confirm;
    if (confirm != 'y') {
        std::cout << CYAN << "Installation cancelled." << NC << std::endl;
        exit(1);
    }

    // Install required tools
    execute_command("apk add btrfs-progs parted dosfstools efibootmgr");
    execute_command("modprobe btrfs");

    // Determine partition names based on disk type
    std::string part1, part2;
    if (config.target_disk.find("nvme") != std::string::npos) {
        part1 = config.target_disk + "p1";
        part2 = config.target_disk + "p2";
    } else {
        part1 = config.target_disk + "1";
        part2 = config.target_disk + "2";
    }

    // Partitioning
    execute_command("parted -s " + config.target_disk + " mklabel gpt");
    execute_command("parted -s " + config.target_disk + " mkpart primary 1MiB 513MiB");
    execute_command("parted -s " + config.target_disk + " set 1 esp on");
    execute_command("parted -s " + config.target_disk + " mkpart primary 513MiB 100%");

    // Formatting
    if (config.boot_filesystem == "fat32") {
        execute_command("mkfs.vfat -F32 " + part1);
    } else if (config.boot_filesystem == "ext4") {
        execute_command("mkfs.ext4 " + part1);
    }
    execute_command("mkfs.btrfs -f " + part2);

    // Mounting and subvolumes
    execute_command("mount " + part2 + " /mnt");
    execute_command("btrfs subvolume create /mnt/@");
    execute_command("btrfs subvolume create /mnt/@home");
    execute_command("btrfs subvolume create /mnt/@root");
    execute_command("btrfs subvolume create /mnt/@srv");
    execute_command("btrfs subvolume create /mnt/@tmp");
    execute_command("btrfs subvolume create /mnt/@log");
    execute_command("btrfs subvolume create /mnt/@cache");
    execute_command("umount /mnt");

    // Remount with compression
    std::string compress_options = "compress=zstd:" + std::to_string(config.compression_level) + 
                                 ",compress-force=zstd:" + std::to_string(config.compression_level);
    execute_command("mount -o subvol=@," + compress_options + " " + part2 + " /mnt");
    execute_command("mkdir -p /mnt/boot/efi");
    execute_command("mount " + part1 + " /mnt/boot/efi");
    execute_command("mkdir -p /mnt/home");
    execute_command("mkdir -p /mnt/root");
    execute_command("mkdir -p /mnt/srv");
    execute_command("mkdir -p /mnt/tmp");
    execute_command("mkdir -p /mnt/var/cache");
    execute_command("mkdir -p /mnt/var/log");
    execute_command("mount -o subvol=@home," + compress_options + " " + part2 + " /mnt/home");
    execute_command("mount -o subvol=@root," + compress_options + " " + part2 + " /mnt/root");
    execute_command("mount -o subvol=@srv," + compress_options + " " + part2 + " /mnt/srv");
    execute_command("mount -o subvol=@tmp," + compress_options + " " + part2 + " /mnt/tmp");
    execute_command("mount -o subvol=@log," + compress_options + " " + part2 + " /mnt/var/log");
    execute_command("mount -o subvol=@cache," + compress_options + " " + part2 + " /mnt/var/cache");

    // Install base system
    execute_command("setup-disk -m sys /mnt");

    // Mount required filesystems for chroot
    execute_command("mount -t proc none /mnt/proc");
    execute_command("mount --rbind /dev /mnt/dev");
    execute_command("mount --rbind /sys /mnt/sys");

    // Determine login manager based on desktop environment
    std::string login_manager = "none";
    if (config.desktop_env == "KDE Plasma") {
        login_manager = "sddm";
    } else if (config.desktop_env == "GNOME") {
        login_manager = "gdm";
    } else if (config.desktop_env == "XFCE" || config.desktop_env == "MATE" || config.desktop_env == "LXQt") {
        login_manager = "lightdm";
    }

    // Create chroot setup script
    std::ofstream chroot_script("/mnt/setup-chroot.sh");
    if (chroot_script.is_open()) {
        chroot_script << "#!/bin/ash\n\n";
        chroot_script << "# Basic system configuration\n";
        chroot_script << "echo \"root:" << config.root_password << "\" | chpasswd\n";
        chroot_script << "adduser -D " << config.user_name << " -G wheel,video,audio,input\n";
        chroot_script << "echo \"" << config.user_name << ":" << config.user_password << "\" | chpasswd\n";
        chroot_script << "setup-timezone -z " << config.timezone << "\n";
        chroot_script << "setup-keymap " << config.keymap << " " << config.keymap << "\n";
        chroot_script << "echo \"" << config.hostname << "\" > /etc/hostname\n\n";

        // Generate fstab
        chroot_script << "# Generate fstab\n";
        chroot_script << "cat << EOF > /etc/fstab\n";
        chroot_script << part1 << " /boot/efi " << config.boot_filesystem << " defaults 0 2\n";
        chroot_script << part2 << " / btrfs rw,noatime," << compress_options << ",subvol=@ 0 1\n";
        chroot_script << part2 << " /home btrfs rw,noatime," << compress_options << ",subvol=@home 0 2\n";
        chroot_script << part2 << " /root btrfs rw,noatime," << compress_options << ",subvol=@root 0 2\n";
        chroot_script << part2 << " /srv btrfs rw,noatime," << compress_options << ",subvol=@srv 0 2\n";
        chroot_script << part2 << " /tmp btrfs rw,noatime," << compress_options << ",subvol=@tmp 0 2\n";
        chroot_script << part2 << " /var/log btrfs rw,noatime," << compress_options << ",subvol=@log 0 2\n";
        chroot_script << part2 << " /var/cache btrfs rw,noatime," << compress_options << ",subvol=@cache 0 2\n";
        chroot_script << "EOF\n\n";

        // Update repositories and install desktop environment
        chroot_script << "# Update repositories and install desktop environment\n";
        chroot_script << "apk update\n";
        
        if (config.desktop_env == "KDE Plasma") {
            chroot_script << "setup-desktop plasma\n";
            chroot_script << "apk add plasma-nm\n";
        } else if (config.desktop_env == "GNOME") {
            chroot_script << "setup-desktop gnome\n";
            chroot_script << "apk add networkmanager-gnome\n";
        } else if (config.desktop_env == "XFCE") {
            chroot_script << "setup-desktop xfce\n";
            chroot_script << "apk add networkmanager-gtk\n";
        } else if (config.desktop_env == "MATE") {
            chroot_script << "setup-desktop mate\n";
            chroot_script << "apk add networkmanager-gtk\n";
        } else if (config.desktop_env == "LXQt") {
            chroot_script << "setup-desktop lxqt\n";
            chroot_script << "apk add networkmanager-qt\n";
        } else {
            chroot_script << "echo \"No desktop environment selected\"\n";
            chroot_script << "apk add networkmanager\n";
        }
        chroot_script << "\n";

        // Install bootloader
        chroot_script << "# Install bootloader\n";
        if (config.bootloader == "GRUB") {
            chroot_script << "apk add grub-efi\n";
            chroot_script << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE\n";
            chroot_script << "grub-mkconfig -o /boot/grub/grub.cfg\n";
        } else if (config.bootloader == "rEFInd") {
            chroot_script << "apk add refind\n";
            chroot_script << "refind-install\n";
        }
        chroot_script << "\n";

        // Configure init system
        chroot_script << "# Configure init system\n";
        if (config.init_system == "OpenRC") {
            chroot_script << "rc-update add dbus\n";
            chroot_script << "rc-update add networkmanager\n";
            if (login_manager != "none") {
                chroot_script << "rc-update add " << login_manager << "\n";
            }
        } else if (config.init_system == "sysvinit") {
            chroot_script << "apk add sysvinit openrc\n";
            chroot_script << "ln -sf /etc/inittab.sysvinit /etc/inittab\n";
            chroot_script << "for service in dbus networkmanager " << login_manager << "; do\n";
            chroot_script << "    if [ -f \"/etc/init.d/$service\" ]; then\n";
            chroot_script << "        ln -s /etc/init.d/$service /etc/rc.d/\n";
            chroot_script << "    fi\n";
            chroot_script << "done\n";
        } else if (config.init_system == "runit") {
            chroot_script << "apk add runit runit-openrc\n";
            chroot_script << "mkdir -p /etc/service\n";
            chroot_script << "if [ -f \"/etc/init.d/dbus\" ]; then\n";
            chroot_script << "    mkdir -p /etc/service/dbus\n";
            chroot_script << "    echo '#!/bin/sh' > /etc/service/dbus/run\n";
            chroot_script << "    echo 'exec /etc/init.d/dbus start' >> /etc/service/dbus/run\n";
            chroot_script << "    chmod +x /etc/service/dbus/run\n";
            chroot_script << "fi\n";
        } else if (config.init_system == "s6") {
            chroot_script << "apk add s6 s6-openrc\n";
            chroot_script << "mkdir -p /etc/s6/sv\n";
            chroot_script << "if [ -f \"/etc/init.d/dbus\" ]; then\n";
            chroot_script << "    mkdir -p /etc/s6/sv/dbus\n";
            chroot_script << "    echo '#!/bin/sh' > /etc/s6/sv/dbus/run\n";
            chroot_script << "    echo 'exec /etc/init.d/dbus start' >> /etc/s6/sv/dbus/run\n";
            chroot_script << "    chmod +x /etc/s6/sv/dbus/run\n";
            chroot_script << "fi\n";
        }
        chroot_script << "\n";

        // Clean up
        chroot_script << "# Clean up\n";
        chroot_script << "rm /setup-chroot.sh\n";

        chroot_script.close();
    }

    // Make chroot script executable and run it
    execute_command("chmod +x /mnt/setup-chroot.sh");
    execute_command("chroot /mnt /setup-chroot.sh");

    execute_command("umount -l /mnt");
    std::cout << CYAN << "Installation complete!" << NC << std::endl;

    // Post-install menu
    while (true) {
        const char *choices[] = {
            "Reboot now",
            "Chroot into installed system",
            "Exit without rebooting",
            NULL
        };

        int choice = dialog_menu("Installation Complete", "Select post-install action:", choices, 0);
        
        switch (choice) {
            case 0: // Reboot
                std::cout << CYAN << "Rebooting system..." << NC << std::endl;
                execute_command("reboot");
                break;
            case 1: // Chroot
                std::cout << CYAN << "Entering chroot..." << NC << std::endl;
                execute_command("mount " + part1 + " /mnt/boot/efi");
                execute_command("mount -o subvol=@ " + part2 + " /mnt");
                execute_command("mount -t proc none /mnt/proc");
                execute_command("mount --rbind /dev /mnt/dev");
                execute_command("mount --rbind /sys /mnt/sys");
                execute_command("mount --rbind /dev/pts /mnt/dev/pts");
                execute_command("chroot /mnt /bin/ash");
                execute_command("umount -l /mnt");
                break;
            case 2: // Exit
                exit(0);
            default:
                std::cout << CYAN << "Invalid option selected" << NC << std::endl;
                break;
        }
    }
}

void configure_installation() {
    show_ascii();

    // Get available disks
    auto disks = get_available_disks();
    std::vector<const char*> disk_choices;
    for (const auto& disk : disks) {
        disk_choices.push_back(disk.c_str());
    }
    disk_choices.push_back(nullptr);
    
    int disk_idx = dialog_menu("Target Disk", "Select target disk:", disk_choices.data(), 0);
    if (disk_idx >= 0 && disk_idx < disks.size()) {
        config.target_disk = disks[disk_idx];
    } else {
        config.target_disk = "/dev/nvme0n1"; // Default
    }

    // Hostname
    config.hostname = dialog_inputbox("Hostname", "Enter hostname:", "alpine", 8, 40);

    // Timezone
    config.timezone = dialog_inputbox("Timezone", "Enter timezone (e.g. UTC):", "UTC", 8, 40);

    // Keymap
    config.keymap = dialog_inputbox("Keymap", "Enter keymap (e.g. us):", "us", 8, 40);

    // Username
    config.user_name = dialog_inputbox("Username", "Enter username:", "user", 8, 40);

    // Passwords
    config.user_password = dialog_passwordbox("User Password", "Enter user password:", 8, 40);
    config.root_password = dialog_passwordbox("Root Password", "Enter root password:", 8, 40);

    // Desktop environment
    const char *desktop_choices[] = {
        "KDE Plasma",
        "GNOME",
        "XFCE",
        "MATE",
        "LXQt",
        "None",
        NULL
    };
    int desktop_idx = dialog_menu("Desktop Environment", "Select desktop environment:", desktop_choices, 0);
    if (desktop_idx >= 0 && desktop_idx < 6) {
        config.desktop_env = desktop_choices[desktop_idx];
    } else {
        config.desktop_env = "None";
    }

    // Bootloader
    const char *bootloader_choices[] = {
        "GRUB",
        "rEFInd",
        NULL
    };
    int bootloader_idx = dialog_menu("Bootloader", "Select bootloader:", bootloader_choices, 0);
    if (bootloader_idx >= 0 && bootloader_idx < 2) {
        config.bootloader = bootloader_choices[bootloader_idx];
    } else {
        config.bootloader = "GRUB";
    }

    // Init system
    const char *init_choices[] = {
        "OpenRC",
        "sysvinit",
        "runit",
        "s6",
        NULL
    };
    int init_idx = dialog_menu("Init System", "Select init system:", init_choices, 0);
    if (init_idx >= 0 && init_idx < 4) {
        config.init_system = init_choices[init_idx];
    } else {
        config.init_system = "OpenRC";
    }

    // Boot filesystem
    const char *fs_choices[] = {
        "fat32",
        "ext4",
        NULL
    };
    int fs_idx = dialog_menu("Boot Filesystem", "Select boot filesystem:", fs_choices, 0);
    if (fs_idx >= 0 && fs_idx < 2) {
        config.boot_filesystem = fs_choices[fs_idx];
    } else {
        config.boot_filesystem = "fat32";
    }

    // Compression level
    std::string compression_str = dialog_inputbox("Compression Level", "Enter BTRFS compression level (1-22, default is 22):", "22", 8, 40);
    try {
        config.compression_level = std::stoi(compression_str);
        if (config.compression_level < 1 || config.compression_level > 22) {
            dialog_msgbox("Invalid Compression Level", "Using default value (22).", 6, 40);
            config.compression_level = 22;
        }
    } catch (...) {
        dialog_msgbox("Invalid Compression Level", "Using default value (22).", 6, 40);
        config.compression_level = 22;
    }
}

void main_menu() {
    while (true) {
        const char *choices[] = {
            "Configure Installation",
            "Find Fastest Mirrors",
            "Start Installation",
            "Exit",
            NULL
        };

        int choice = dialog_menu("Alpine Btrfs Installer v1.02 12-07-2025", "Select option:", choices, 0);
        
        switch (choice) {
            case 0: // Configure
                configure_installation();
                break;
            case 1: // Mirrors
                configure_fastest_mirrors();
                break;
            case 2: // Install
                if (config.target_disk.empty()) {
                    dialog_msgbox("Error", "Please configure installation first!", 6, 40);
                } else {
                    perform_installation();
                }
                break;
            case 3: // Exit
                exit(0);
            default:
                break;
        }
    }
}

int main() {
    // Initialize dialog
    dialog_init();
    
    // Set default configuration
    config.compression_level = 22;
    config.boot_filesystem = "fat32";
    
    show_ascii();
    main_menu();
    
    // Clean up dialog
    dialog_cleanup();
    return 0;
}
