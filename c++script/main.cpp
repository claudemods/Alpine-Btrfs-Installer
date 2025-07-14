#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <algorithm>

using namespace std;

// Color definitions
#define RED "\033[38;2;255;0;0m"
#define CYAN "\033[38;2;0;255;255m"
#define NC "\033[0m"

// Global configuration variables
string TARGET_DISK;
string HOSTNAME;
string TIMEZONE;
string KEYMAP;
string USER_NAME;
string USER_PASSWORD;
string ROOT_PASSWORD;
string DESKTOP_ENV;
string BOOTLOADER;
string INIT_SYSTEM;
string BOOT_FS_TYPE;
int COMPRESSION_LEVEL;

void show_ascii() {
    system("clear");
    cout << RED
         << "░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗\n"
         << "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝\n"
         << "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░\n"
         << "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗\n"
         << "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝\n"
         << "░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░" 
         << NC << endl;
    cout << CYAN << "Alpine Btrfs Installer v1.02 12-07-2025" << NC << endl << endl;
}

void execute_command(const string& cmd, bool show_output = true) {
    if (show_output) {
        cout << CYAN << "[EXEC] " << cmd << NC << endl;
    }
    int result = system(cmd.c_str());
    if (result != 0 && show_output) {
        cout << RED << "Command failed: " << cmd << NC << endl;
    }
}

string get_dialog_input(const string& title, const string& prompt, const string& default_val = "") {
    string cmd = "dialog --title \"" + title + "\" --inputbox \"" + prompt + "\" 10 50";
    if (!default_val.empty()) {
        cmd += " " + default_val;
    }
    cmd += " 2>&1 >/dev/tty";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        cerr << "Error opening pipe to dialog" << endl;
        return "";
    }

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // Remove trailing newline
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }

    return result;
}

string get_dialog_password(const string& title, const string& prompt) {
    string cmd = "dialog --title \"" + title + "\" --passwordbox \"" + prompt + "\" 10 50 2>&1 >/dev/tty";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        cerr << "Error opening pipe to dialog" << endl;
        return "";
    }

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // Remove trailing newline
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }

    return result;
}

int get_dialog_menu(const string& title, const string& prompt, const vector<pair<string, string>>& options) {
    string cmd = "dialog --title \"" + title + "\" --menu \"" + prompt + "\" 15 40 " + to_string(options.size());
    for (const auto& opt : options) {
        cmd += " \"" + opt.first + "\" \"" + opt.second + "\"";
    }
    cmd += " 2>&1 >/dev/tty";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        cerr << "Error opening pipe to dialog" << endl;
        return -1;
    }

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // Remove trailing newline
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }

    if (result.empty()) return -1;

    // Find which option was selected
    for (size_t i = 0; i < options.size(); i++) {
        if (options[i].first == result) {
            return i;
        }
    }

    return -1;
}

bool get_dialog_yesno(const string& title, const string& prompt) {
    string cmd = "dialog --title \"" + title + "\" --yesno \"" + prompt + "\" 10 50 2>&1 >/dev/tty";
    int result = system(cmd.c_str());
    return (result == 0);
}

void configure_fastest_mirrors() {
    show_ascii();
    if (get_dialog_yesno("Fastest Mirrors", "Would you like to find and use the fastest mirrors?")) {
        cout << CYAN << "Finding fastest mirrors..." << NC << endl;
        execute_command("apk add reflector", false);
        execute_command("reflector --latest 20 --protocol https --sort rate --save /etc/apk/repositories");
        cout << CYAN << "Mirrorlist updated with fastest mirrors" << NC << endl;
    } else {
        cout << CYAN << "Using default mirrors" << NC << endl;
    }
}

void perform_installation() {
    show_ascii();

    // Check root
    if (getuid() != 0) {
        cout << CYAN << "This script must be run as root or with sudo" << NC << endl;
        exit(1);
    }

    // Check UEFI
    if (access("/sys/firmware/efi", F_OK) == -1) {
        cout << CYAN << "ERROR: This script requires UEFI boot mode" << NC << endl;
        exit(1);
    }

    // Confirm installation
    cout << CYAN << "About to install to " << TARGET_DISK << " with these settings:" << endl;
    cout << "Hostname: " << HOSTNAME << endl;
    cout << "Timezone: " << TIMEZONE << endl;
    cout << "Keymap: " << KEYMAP << endl;
    cout << "Username: " << USER_NAME << endl;
    cout << "Desktop: " << DESKTOP_ENV << endl;
    cout << "Bootloader: " << BOOTLOADER << endl;
    cout << "Init System: " << INIT_SYSTEM << endl;
    cout << "Boot Filesystem: " << BOOT_FS_TYPE << endl;
    cout << "Compression Level: " << COMPRESSION_LEVEL << NC << endl;
    
    if (!get_dialog_yesno("Confirmation", "Continue with installation?")) {
        cout << CYAN << "Installation cancelled." << NC << endl;
        exit(1);
    }

    // Install required tools
    execute_command("apk add btrfs-progs parted dosfstools efibootmgr");
    execute_command("modprobe btrfs");

    // Partitioning
    execute_command("parted -s " + TARGET_DISK + " mklabel gpt");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 1MiB 513MiB");
    execute_command("parted -s " + TARGET_DISK + " set 1 esp on");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 513MiB 100%");

    // Formatting
    string boot_part = TARGET_DISK;
    if (boot_part.find("nvme") != string::npos) {
        boot_part += "p1";
    } else {
        boot_part += "1";
    }

    string root_part = TARGET_DISK;
    if (root_part.find("nvme") != string::npos) {
        root_part += "p2";
    } else {
        root_part += "2";
    }

    if (BOOT_FS_TYPE == "fat32") {
        execute_command("mkfs.vfat -F32 " + boot_part);
    } else if (BOOT_FS_TYPE == "ext4") {
        execute_command("mkfs.ext4 " + boot_part);
    }
    
    execute_command("mkfs.btrfs -f " + root_part);

    // Mounting and subvolumes
    execute_command("mount " + root_part + " /mnt");
    execute_command("btrfs subvolume create /mnt/@");
    execute_command("btrfs subvolume create /mnt/@home");
    execute_command("btrfs subvolume create /mnt/@root");
    execute_command("btrfs subvolume create /mnt/@srv");
    execute_command("btrfs subvolume create /mnt/@tmp");
    execute_command("btrfs subvolume create /mnt/@log");
    execute_command("btrfs subvolume create /mnt/@cache");
    execute_command("umount /mnt");

    // Remount with compression
    execute_command("mount -o subvol=@,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt");
    execute_command("mkdir -p /mnt/boot/efi");
    execute_command("mount " + boot_part + " /mnt/boot/efi");
    execute_command("mkdir -p /mnt/home");
    execute_command("mkdir -p /mnt/root");
    execute_command("mkdir -p /mnt/srv");
    execute_command("mkdir -p /mnt/tmp");
    execute_command("mkdir -p /mnt/var/cache");
    execute_command("mkdir -p /mnt/var/log");
    execute_command("mount -o subvol=@home,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/home");
    execute_command("mount -o subvol=@root,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/root");
    execute_command("mount -o subvol=@srv,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/srv");
    execute_command("mount -o subvol=@tmp,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/tmp");
    execute_command("mount -o subvol=@log,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/var/log");
    execute_command("mount -o subvol=@cache,compress=zstd:" + to_string(COMPRESSION_LEVEL) + 
                   ",compress-force=zstd:" + to_string(COMPRESSION_LEVEL) + " " + root_part + " /mnt/var/cache");

    // Install base system
    execute_command("setup-disk -m sys /mnt");

    // Mount required filesystems for chroot
    execute_command("mount -t proc none /mnt/proc");
    execute_command("mount --rbind /dev /mnt/dev");
    execute_command("mount --rbind /sys /mnt/sys");

    // Determine login manager
    string LOGIN_MANAGER = "none";
    if (DESKTOP_ENV == "KDE Plasma") {
        LOGIN_MANAGER = "sddm";
    } else if (DESKTOP_ENV == "GNOME") {
        LOGIN_MANAGER = "gdm";
    } else if (DESKTOP_ENV == "XFCE" || DESKTOP_ENV == "MATE" || DESKTOP_ENV == "LXQt") {
        LOGIN_MANAGER = "lightdm";
    }

    // Create chroot setup script
    ofstream chroot_script("/mnt/setup-chroot.sh");
    chroot_script << "#!/bin/ash\n\n";
    chroot_script << "# Basic system configuration\n";
    chroot_script << "echo \"root:" << ROOT_PASSWORD << "\" | chpasswd\n";
    chroot_script << "adduser -D " << USER_NAME << " -G wheel,video,audio,input\n";
    chroot_script << "echo \"" << USER_NAME << ":" << USER_PASSWORD << "\" | chpasswd\n";
    chroot_script << "setup-timezone -z " << TIMEZONE << "\n";
    chroot_script << "setup-keymap " << KEYMAP << " " << KEYMAP << "\n";
    chroot_script << "echo \"" << HOSTNAME << "\" > /etc/hostname\n\n";

    chroot_script << "# Generate fstab\n";
    chroot_script << "cat << EOF > /etc/fstab\n";
    chroot_script << boot_part << " /boot/efi " << BOOT_FS_TYPE << " defaults 0 2\n";
    chroot_script << root_part << " / btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@ 0 1\n";
    chroot_script << root_part << " /home btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@home 0 2\n";
    chroot_script << root_part << " /root btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@root 0 2\n";
    chroot_script << root_part << " /srv btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@srv 0 2\n";
    chroot_script << root_part << " /tmp btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@tmp 0 2\n";
    chroot_script << root_part << " /var/log btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@log 0 2\n";
    chroot_script << root_part << " /var/cache btrfs rw,noatime,compress=zstd:" << COMPRESSION_LEVEL << ",compress-force=zstd:" << COMPRESSION_LEVEL << ",subvol=@cache 0 2\n";
    chroot_script << "EOF\n\n";

    chroot_script << "# Update repositories and install desktop environment\n";
    chroot_script << "apk update\n";
    
    if (DESKTOP_ENV == "KDE Plasma") {
        chroot_script << "setup-desktop plasma\n";
        chroot_script << "apk add plasma-nm\n";
    } else if (DESKTOP_ENV == "GNOME") {
        chroot_script << "setup-desktop gnome\n";
        chroot_script << "apk add networkmanager-gnome\n";
    } else if (DESKTOP_ENV == "XFCE") {
        chroot_script << "setup-desktop xfce\n";
        chroot_script << "apk add networkmanager-gtk\n";
    } else if (DESKTOP_ENV == "MATE") {
        chroot_script << "setup-desktop mate\n";
        chroot_script << "apk add networkmanager-gtk\n";
    } else if (DESKTOP_ENV == "LXQt") {
        chroot_script << "setup-desktop lxqt\n";
        chroot_script << "apk add networkmanager-qt\n";
    } else {
        chroot_script << "echo \"No desktop environment selected\"\n";
        chroot_script << "apk add networkmanager\n";
    }
    chroot_script << "\n";

    // Install bootloader
    if (BOOTLOADER == "GRUB") {
        chroot_script << "apk add grub-efi\n";
        chroot_script << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE\n";
        chroot_script << "grub-mkconfig -o /boot/grub/grub.cfg\n";
    } else if (BOOTLOADER == "rEFInd") {
        chroot_script << "apk add refind\n";
        chroot_script << "refind-install\n";
    }
    chroot_script << "\n";

    // Configure init system
    if (INIT_SYSTEM == "OpenRC") {
        chroot_script << "rc-update add dbus\n";
        chroot_script << "rc-update add networkmanager\n";
        if (LOGIN_MANAGER != "none") {
            chroot_script << "rc-update add " << LOGIN_MANAGER << "\n";
        }
    } else if (INIT_SYSTEM == "sysvinit") {
        chroot_script << "apk add sysvinit openrc\n";
        chroot_script << "ln -sf /etc/inittab.sysvinit /etc/inittab\n";
        chroot_script << "if [ -f \"/etc/init.d/dbus\" ]; then\n";
        chroot_script << "    ln -s /etc/init.d/dbus /etc/rc.d/\n";
        chroot_script << "fi\n";
        chroot_script << "if [ -f \"/etc/init.d/networkmanager\" ]; then\n";
        chroot_script << "    ln -s /etc/init.d/networkmanager /etc/rc.d/\n";
        chroot_script << "fi\n";
        if (LOGIN_MANAGER != "none") {
            chroot_script << "if [ -f \"/etc/init.d/" << LOGIN_MANAGER << "\" ]; then\n";
            chroot_script << "    ln -s /etc/init.d/" << LOGIN_MANAGER << " /etc/rc.d/\n";
            chroot_script << "fi\n";
        }
    } else if (INIT_SYSTEM == "runit") {
        chroot_script << "apk add runit runit-openrc\n";
        chroot_script << "mkdir -p /etc/service\n";
        chroot_script << "if [ -f \"/etc/init.d/dbus\" ]; then\n";
        chroot_script << "    mkdir -p /etc/service/dbus\n";
        chroot_script << "    echo '#!/bin/sh' > /etc/service/dbus/run\n";
        chroot_script << "    echo 'exec /etc/init.d/dbus start' >> /etc/service/dbus/run\n";
        chroot_script << "    chmod +x /etc/service/dbus/run\n";
        chroot_script << "fi\n";
    } else if (INIT_SYSTEM == "s6") {
        chroot_script << "apk add s6 s6-openrc\n";
        chroot_script << "mkdir -p /etc/s6/sv\n";
        chroot_script << "if [ -f \"/etc/init.d/dbus\" ]; then\n";
        chroot_script << "    mkdir -p /etc/s6/sv/dbus\n";
        chroot_script << "    echo '#!/bin/sh' > /etc/s6/sv/dbus/run\n";
        chroot_script << "    echo 'exec /etc/init.d/dbus start' >> /etc/s6/sv/dbus/run\n";
        chroot_script << "    chmod +x /etc/s6/sv/dbus/run\n";
        chroot_script << "fi\n";
    }
    chroot_script << "\n# Clean up\nrm /setup-chroot.sh\n";
    chroot_script.close();

    // Make script executable and run it
    execute_command("chmod +x /mnt/setup-chroot.sh");
    execute_command("chroot /mnt /setup-chroot.sh");

    // Unmount everything
    execute_command("umount -R /mnt");
    cout << CYAN << "Installation complete!" << NC << endl;

    // Post-install menu
    while (true) {
        vector<pair<string, string>> options = {
            {"1", "Reboot now"},
            {"2", "Chroot into installed system"},
            {"3", "Exit without rebooting"}
        };

        int choice = get_dialog_menu("Installation Complete", "Select post-install action:", options);
        
        switch (choice) {
            case 0: // Reboot
                cout << CYAN << "Rebooting system..." << NC << endl;
                execute_command("reboot");
                break;
            case 1: // Chroot
                cout << CYAN << "Entering chroot..." << NC << endl;
                execute_command("mount " + boot_part + " /mnt/boot/efi");
                execute_command("mount -o subvol=@ " + root_part + " /mnt");
                execute_command("mount -t proc none /mnt/proc");
                execute_command("mount --rbind /dev /mnt/dev");
                execute_command("mount --rbind /sys /mnt/sys");
                execute_command("mount --rbind /dev/pts /mnt/dev/pts");
                execute_command("chroot /mnt /bin/ash");
                execute_command("umount -R /mnt");
                break;
            case 2: // Exit
                cout << CYAN << "Exiting installer." << NC << endl;
                exit(0);
            default:
                cout << CYAN << "Invalid option selected" << NC << endl;
                break;
        }
    }
}

void configure_installation() {
    show_ascii();

    // Get target disk
    TARGET_DISK = get_dialog_input("Target Disk", "Enter target disk (e.g. /dev/nvme0n1, /dev/sda):");
    if (TARGET_DISK.empty()) {
        cout << RED << "Target disk is required!" << NC << endl;
        exit(1);
    }

    // Get boot filesystem type
    vector<pair<string, string>> fs_options = {
        {"fat32", "FAT32 (Recommended for UEFI)"},
        {"ext4", "EXT4 (Alternative)"}
    };
    int fs_choice = get_dialog_menu("Boot Filesystem", "Select boot partition filesystem:", fs_options);
    if (fs_choice == 0) {
        BOOT_FS_TYPE = "fat32";
    } else if (fs_choice == 1) {
        BOOT_FS_TYPE = "ext4";
    } else {
        cout << RED << "Boot filesystem selection is required!" << NC << endl;
        exit(1);
    }

    // Get other configuration
    HOSTNAME = get_dialog_input("Hostname", "Enter hostname:");
    TIMEZONE = get_dialog_input("Timezone", "Enter timezone (e.g. America/New_York):");
    KEYMAP = get_dialog_input("Keymap", "Enter keymap (e.g. us):");
    USER_NAME = get_dialog_input("Username", "Enter username:");
    USER_PASSWORD = get_dialog_password("User Password", "Enter user password:");
    ROOT_PASSWORD = get_dialog_password("Root Password", "Enter root password:");

    // Get desktop environment
    vector<pair<string, string>> de_options = {
        {"KDE Plasma", "KDE Plasma Desktop"},
        {"GNOME", "GNOME Desktop"},
        {"XFCE", "XFCE Desktop"},
        {"MATE", "MATE Desktop"},
        {"LXQt", "LXQt Desktop"},
        {"None", "No desktop environment"}
    };
    int de_choice = get_dialog_menu("Desktop Environment", "Select desktop environment:", de_options);
    if (de_choice >= 0 && de_choice < de_options.size()) {
        DESKTOP_ENV = de_options[de_choice].first;
    } else {
        DESKTOP_ENV = "None";
    }

    // Get bootloader
    vector<pair<string, string>> bl_options = {
        {"GRUB", "GRUB (Recommended)"},
        {"rEFInd", "Graphical boot manager"}
    };
    int bl_choice = get_dialog_menu("Bootloader", "Select bootloader:", bl_options);
    if (bl_choice == 0) {
        BOOTLOADER = "GRUB";
    } else if (bl_choice == 1) {
        BOOTLOADER = "rEFInd";
    } else {
        BOOTLOADER = "GRUB";
    }

    // Get init system
    vector<pair<string, string>> init_options = {
        {"OpenRC", "Alpine's default init system"},
        {"sysvinit", "Traditional System V init"},
        {"runit", "Runit init system"},
        {"s6", "s6 init system"}
    };
    int init_choice = get_dialog_menu("Init System", "Select init system:", init_options);
    if (init_choice >= 0 && init_choice < init_options.size()) {
        INIT_SYSTEM = init_options[init_choice].first;
    } else {
        INIT_SYSTEM = "OpenRC";
    }

    // Get compression level
    string comp_level = get_dialog_input("Compression Level", "Enter BTRFS compression level (1-22):");
    try {
        COMPRESSION_LEVEL = stoi(comp_level);
        if (COMPRESSION_LEVEL < 1 || COMPRESSION_LEVEL > 22) {
            cout << RED << "Invalid compression level. Using default (3)." << NC << endl;
            COMPRESSION_LEVEL = 3;
        }
    } catch (...) {
        cout << RED << "Invalid compression level. Using default (3)." << NC << endl;
        COMPRESSION_LEVEL = 3;
    }
}

void main_menu() {
    while (true) {
        vector<pair<string, string>> options = {
            {"1", "Configure Installation"},
            {"2", "Find Fastest Mirrors"},
            {"3", "Start Installation"},
            {"4", "Exit"}
        };

        int choice = get_dialog_menu("Alpine Btrfs Installer v1.02 12-07-2025", "Select option:", options);
        
        switch (choice) {
            case 0: // Configure
                configure_installation();
                break;
            case 1: // Mirrors
                configure_fastest_mirrors();
                break;
            case 2: // Install
                if (TARGET_DISK.empty()) {
                    cout << RED << "Please configure installation first!" << NC << endl;
                } else {
                    perform_installation();
                }
                break;
            case 3: // Exit
                cout << CYAN << "Exiting installer." << NC << endl;
                exit(0);
            default:
                cout << CYAN << "Invalid option selected" << NC << endl;
                break;
        }
    }
}

int main() {
    show_ascii();
    main_menu();
    return 0;
}
