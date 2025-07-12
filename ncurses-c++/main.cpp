#include <ncurses.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <locale.h>

using namespace std;

// Renamed ANSI color codes to avoid conflicts with ncurses
#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_RESET   "\033[0m"

// Global configuration
string TARGET_DISK, HOSTNAME, TIMEZONE, KEYMAP;
string USER_NAME, USER_PASSWORD, ROOT_PASSWORD;
string DESKTOP_ENV, LOGIN_MANAGER;

// Function to run commands with color and sudo
void execute_command(const string& cmd) {
    cout << ANSI_CYAN;
    fflush(stdout);
    string full_cmd = "doas " + cmd;
    int status = system(full_cmd.c_str());
    cout << ANSI_RESET;
    if (status != 0) {
        cerr << ANSI_RED << "Error executing: " << full_cmd << ANSI_RESET << endl;
        exit(1);
    }
}

string exec(const char* cmd) {
    char buffer[128];
    string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
    pclose(pipe);
    return result;
}

void show_ascii() {
    clear();
    attron(COLOR_PAIR(1));
    printw(R"(
░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░
    )");
    attron(COLOR_PAIR(2));
    printw("claudemods v1.01 11-07-2025\n\n");
    refresh();
}

void show_message(const string& title, const string& message) {
    clear();
    show_ascii();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int width = max(message.length(), title.length()) + 4;
    width = min(width, cols - 4);
    int height = 5;
    int starty = (rows - height) / 2;
    int startx = (cols - width) / 2;

    WINDOW* win = newwin(height, width, starty, startx);
    wbkgd(win, COLOR_PAIR(3));
    wattron(win, COLOR_PAIR(3));
    box(win, 0, 0);
    mvwprintw(win, 0, (width - title.length())/2, "%s", title.c_str());
    mvwprintw(win, 2, 2, "%s", message.c_str());
    mvwprintw(win, height-2, (width-10)/2, "[ OK ]");
    wrefresh(win);

    noecho();
    cbreak();
    getch();
    delwin(win);
}

string get_input(const string& title, const string& prompt, const string& default_val = "") {
    clear();
    show_ascii();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int width = max(prompt.length(), title.length()) + 4;
    width = min(width, cols - 4);
    int height = 7;
    int starty = (rows - height) / 2;
    int startx = (cols - width) / 2;

    WINDOW* win = newwin(height, width, starty, startx);
    wbkgd(win, COLOR_PAIR(3));
    wattron(win, COLOR_PAIR(3));
    box(win, 0, 0);
    mvwprintw(win, 0, (width-title.length())/2, "%s", title.c_str());
    mvwprintw(win, 2, 2, "%s", prompt.c_str());

    echo();
    char input[256];
    mvwgetnstr(win, 4, 2, input, sizeof(input)-1);
    noecho();

    string result(input);
    if (result.empty() && !default_val.empty()) result = default_val;
    delwin(win);
    return result;
}

int show_menu(const string& title, const vector<string>& options) {
    clear();
    show_ascii();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int width = title.length() + 4;
    for (const auto& opt : options) width = max(width, (int)opt.length() + 4);
    width = min(width, cols - 4);

    int height = options.size() + 6;
    height = min(height, rows - 2);

    int starty = (rows - height) / 2;
    int startx = (cols - width) / 2;

    WINDOW* win = newwin(height, width, starty, startx);
    wbkgd(win, COLOR_PAIR(3));
    keypad(win, TRUE);
    wattron(win, COLOR_PAIR(3));
    box(win, 0, 0);
    mvwprintw(win, 0, (width-title.length())/2, "%s", title.c_str());

    int highlight = 0;
    while (true) {
        for (size_t i = 0; i < options.size(); i++) {
            if (i == (size_t)highlight) {
                wattron(win, COLOR_PAIR(4) | A_BOLD);
            } else {
                wattron(win, COLOR_PAIR(3));
            }
            mvwprintw(win, i+2, 2, "%s", options[i].c_str());
            wattroff(win, A_BOLD);
        }

        int ch = wgetch(win);
        switch (ch) {
            case KEY_UP: highlight--; break;
            case KEY_DOWN: highlight++; break;
            case 10: delwin(win); return highlight;
            default: break;
        }

        if (highlight < 0) highlight = options.size() - 1;
        if ((size_t)highlight >= options.size()) highlight = 0;
    }
}

string get_password(const string& title, const string& prompt) {
    return get_input(title, prompt);
}

void configure_installation() {
    TARGET_DISK = get_input("Target Disk", "Enter target disk (e.g. /dev/sda):");
    HOSTNAME = get_input("Hostname", "Enter hostname:");
    TIMEZONE = get_input("Timezone", "Enter timezone (e.g. UTC):");
    KEYMAP = get_input("Keymap", "Enter keymap (e.g. us):");
    USER_NAME = get_input("Username", "Enter username:");
    USER_PASSWORD = get_password("User Password", "Enter user password:");
    ROOT_PASSWORD = get_password("Root Password", "Enter root password:");

    vector<string> desktops = {
        "KDE Plasma - KDE Plasma Desktop",
        "GNOME - GNOME Desktop",
        "XFCE - XFCE Desktop",
        "MATE - MATE Desktop",
        "LXQt - LXQt Desktop"
    };

    int choice = show_menu("Desktop Environment", desktops);
    switch (choice) {
        case 0: DESKTOP_ENV = "KDE Plasma"; break;
        case 1: DESKTOP_ENV = "GNOME"; break;
        case 2: DESKTOP_ENV = "XFCE"; break;
        case 3: DESKTOP_ENV = "MATE"; break;
        case 4: DESKTOP_ENV = "LXQt"; break;
    }
}

void perform_installation() {
    show_ascii();

    if (geteuid() != 0) {
        show_message("Error", "This script must be run as root or with sudo");
        return;
    }

    if (access("/sys/firmware/efi", F_OK) == -1) {
        show_message("Error", "ERROR: This script requires UEFI boot mode");
        return;
    }

    stringstream confirm_msg;
    confirm_msg << "About to install to " << TARGET_DISK << " with:\n"
    << "Hostname: " << HOSTNAME << "\n"
    << "Timezone: " << TIMEZONE << "\n"
    << "Keymap: " << KEYMAP << "\n"
    << "Username: " << USER_NAME << "\n"
    << "Desktop: " << DESKTOP_ENV << "\n"
    << "Continue? (y/n):";

    string confirm = get_input("Confirmation", confirm_msg.str());
    if (confirm != "y" && confirm != "Y") {
        show_message("Installation", "Installation cancelled.");
        return;
    }

    // Installation steps
    execute_command("apk add btrfs-progs parted dosfstools grub-efi efibootmgr");
    execute_command("modprobe btrfs");

    execute_command("parted -s " + TARGET_DISK + " mklabel gpt");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 1MiB 513MiB");
    execute_command("parted -s " + TARGET_DISK + " set 1 esp on");
    execute_command("parted -s " + TARGET_DISK + " mkpart primary 513MiB 100%");

    execute_command("mkfs.vfat -F32 " + TARGET_DISK + "1");
    execute_command("mkfs.btrfs -f " + TARGET_DISK + "2");

    execute_command("mount " + TARGET_DISK + "2 /mnt");
    execute_command("btrfs subvolume create /mnt/@");
    execute_command("btrfs subvolume create /mnt/@home");
    execute_command("btrfs subvolume create /mnt/@root");
    execute_command("btrfs subvolume create /mnt/@srv");
    execute_command("btrfs subvolume create /mnt/@tmp");
    execute_command("btrfs subvolume create /mnt/@log");
    execute_command("btrfs subvolume create /mnt/@cache");
    execute_command("umount /mnt");

    execute_command("mount -o subvol=@,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt");
    execute_command("mkdir -p /mnt/boot/efi");
    execute_command("mount " + TARGET_DISK + "1 /mnt/boot/efi");
    execute_command("mkdir -p /mnt/home");
    execute_command("mkdir -p /mnt/root");
    execute_command("mkdir -p /mnt/srv");
    execute_command("mkdir -p /mnt/tmp");
    execute_command("mkdir -p /mnt/var/cache");
    execute_command("mkdir -p /mnt/var/log");
    execute_command("mount -o subvol=@home,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt/home");
    execute_command("mount -o subvol=@root,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt/root");
    execute_command("mount -o subvol=@srv,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt/srv");
    execute_command("mount -o subvol=@tmp,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt/tmp");
    execute_command("mount -o subvol=@log,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt/var/log");
    execute_command("mount -o subvol=@cache,compress=zstd:22,compress-force=zstd:22 " + TARGET_DISK + "2 /mnt/var/cache");

    execute_command("setup-disk -m sys /mnt");

    execute_command("mount -t proc none /mnt/proc");
    execute_command("mount --rbind /dev /mnt/dev");
    execute_command("mount --rbind /sys /mnt/sys");

    if (DESKTOP_ENV == "KDE Plasma") LOGIN_MANAGER = "sddm";
    else if (DESKTOP_ENV == "GNOME") LOGIN_MANAGER = "gdm";
    else if (DESKTOP_ENV == "XFCE" || DESKTOP_ENV == "MATE" || DESKTOP_ENV == "LXQt") LOGIN_MANAGER = "lightdm";
    else LOGIN_MANAGER = "none";

    // Create chroot script
    ofstream chroot("/mnt/setup-chroot.sh");
    chroot << "#!/bin/ash\n"
    << "echo \"root:" << ROOT_PASSWORD << "\" | chpasswd\n"
    << "adduser -D " << USER_NAME << " -G wheel,video,audio,input\n"
    << "echo \"" << USER_NAME << ":" << USER_PASSWORD << "\" | chpasswd\n"
    << "setup-timezone -z " << TIMEZONE << "\n"
    << "setup-keymap " << KEYMAP << " " << KEYMAP << "\n"
    << "echo \"" << HOSTNAME << "\" > /etc/hostname\n\n"
    << "cat << EOF > /etc/fstab\n"
    << TARGET_DISK << "1 /boot/efi vfat defaults 0 2\n"
    << TARGET_DISK << "2 / btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@ 0 1\n"
    << TARGET_DISK << "2 /home btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@home 0 2\n"
    << TARGET_DISK << "2 /root btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@root 0 2\n"
    << TARGET_DISK << "2 /srv btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@srv 0 2\n"
    << TARGET_DISK << "2 /tmp btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@tmp 0 2\n"
    << TARGET_DISK << "2 /var/log btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@log 0 2\n"
    << TARGET_DISK << "2 /var/cache btrfs rw,noatime,compress=zstd:22,compress-force=zstd:22,subvol=@cache 0 2\n"
    << "EOF\n\n"
    << "apk update\n";

    if (DESKTOP_ENV == "KDE Plasma") chroot << "setup-desktop plasma\n";
    else if (DESKTOP_ENV == "GNOME") chroot << "setup-desktop gnome\n";
    else if (DESKTOP_ENV == "XFCE") chroot << "setup-desktop xfce\n";
    else if (DESKTOP_ENV == "MATE") chroot << "setup-desktop mate\n";
    else if (DESKTOP_ENV == "LXQt") chroot << "setup-desktop lxqt\n";

    chroot << "apk add grub-efi efibootmgr\n"
    << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE\n"
    << "grub-mkconfig -o /boot/grub/grub.cfg\n\n"
    << "rc-update add dbus\n"
    << "rc-update add networkmanager\n";
    if (LOGIN_MANAGER != "none") chroot << "rc-update add " << LOGIN_MANAGER << "\n";
    chroot << "rm /setup-chroot.sh\n";
    chroot.close();

    execute_command("chmod +x /mnt/setup-chroot.sh");
    execute_command("chroot /mnt /setup-chroot.sh");

    execute_command("umount -l /mnt");
    show_message("Installation", "Installation complete!");

    while (true) {
        vector<string> options = {
            "Reboot now",
            "Chroot into installed system",
            "Exit without rebooting"
        };

        int choice = show_menu("Post Installation", options);

        switch (choice) {
            case 0: execute_command("reboot"); break;
            case 1:
                execute_command("mount " + TARGET_DISK + "1 /mnt/boot/efi");
                execute_command("mount -o subvol=@ " + TARGET_DISK + "2 /mnt");
                execute_command("mount -t proc none /mnt/proc");
                execute_command("mount --rbind /dev /mnt/dev");
                execute_command("mount --rbind /sys /mnt/sys");
                execute_command("mount --rbind /dev/pts /mnt/dev/pts");
                execute_command("chroot /mnt /bin/ash");
                execute_command("umount -l /mnt");
                break;
            case 2: return;
        }
    }
}

void main_menu() {
    while (true) {
        vector<string> options = {
            "Configure Installation",
            "Start Installation",
            "Exit"
        };

        int choice = show_menu("Alpine Linux Btrfs Installer", options);

        switch (choice) {
            case 0: configure_installation(); break;
            case 1:
                if (TARGET_DISK.empty()) {
                    show_message("Error", "Please configure installation first!");
                } else {
                    perform_installation();
                }
                break;
            case 2: return;
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();

    // Color pairs:
    // 1: RED ASCII art (using ncurses COLOR_RED)
    // 2: WHITE regular text
    // 3: WHITE on BLUE (boxes)
    // 4: WHITE on BLUE (selection highlight)
    // Initialize color pairs
    init_pair(1, COLOR_RED, -1);      // Red for ASCII art
    init_pair(2, COLOR_CYAN, -1);     // Cyan for text
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_WHITE);

    bkgd(COLOR_PAIR(2));

    show_ascii();
    main_menu();

    endwin();
    return 0;
}

