#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QProcess>
#include <QTimer>
#include <QFont>
#include <QFontDatabase>
#include <QGroupBox>
#include <QButtonGroup>
#include <QFileDialog>
#include <QScrollArea>
#include <QSettings>
#include <QDebug>
#include <QRegularExpression>

class AlpineInstaller : public QMainWindow {
    Q_OBJECT

public:
    AlpineInstaller(QWidget *parent = nullptr) : QMainWindow(parent) {
        // Set window properties
        setWindowTitle("Alpine Linux BTRFS Installer v1.02");
        resize(900, 700);
        
        // Create central widget and layout
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        
        // Create ASCII art label
        asciiArtLabel = new QLabel(this);
        asciiArtLabel->setAlignment(Qt::AlignCenter);
        asciiArtLabel->setStyleSheet("color: #FF0000; font-family: monospace;");
        
        // Create title label
        titleLabel = new QLabel("Alpine Btrfs Installer v1.02 12-07-2025", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("color: #00FFFF; font-size: 16px; font-weight: bold;");
        
        // Create progress bar
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setStyleSheet("QProgressBar {"
                                   "border: 2px solid grey;"
                                   "border-radius: 5px;"
                                   "text-align: center;"
                                   "}"
                                   "QProgressBar::chunk {"
                                   "background-color: #00FFFF;"
                                   "width: 10px;"
                                   "}");
        
        // Create output console
        outputConsole = new QTextEdit(this);
        outputConsole->setReadOnly(true);
        outputConsole->setStyleSheet("background-color: black; color: #00FFFF; font-family: monospace;");
        
        // Create buttons
        configureButton = new QPushButton("Configure Installation", this);
        mirrorsButton = new QPushButton("Find Fastest Mirrors", this);
        installButton = new QPushButton("Start Installation", this);
        exitButton = new QPushButton("Exit", this);
        
        // Connect buttons
        connect(configureButton, &QPushButton::clicked, this, &AlpineInstaller::configureInstallation);
        connect(mirrorsButton, &QPushButton::clicked, this, &AlpineInstaller::findFastestMirrors);
        connect(installButton, &QPushButton::clicked, this, &AlpineInstaller::startInstallation);
        connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
        
        // Button layout
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(configureButton);
        buttonLayout->addWidget(mirrorsButton);
        buttonLayout->addWidget(installButton);
        buttonLayout->addWidget(exitButton);
        
        // Add widgets to main layout
        mainLayout->addWidget(asciiArtLabel);
        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(progressBar);
        mainLayout->addWidget(outputConsole);
        mainLayout->addLayout(buttonLayout);
        
        setCentralWidget(centralWidget);
        
        // Initialize variables
        initVariables();
        showAsciiArt();
    }

private slots:
    void configureInstallation() {
        QDialog dialog(this);
        dialog.setWindowTitle("Configure Installation");
        dialog.resize(600, 500);
        
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        QScrollArea *scrollArea = new QScrollArea(&dialog);
        QWidget *scrollContent = new QWidget;
        QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
        
        // Disk selection
        QHBoxLayout *diskLayout = new QHBoxLayout();
        diskLayout->addWidget(new QLabel("Target Disk (e.g. /dev/sda):"));
        QLineEdit *diskEdit = new QLineEdit(targetDisk, &dialog);
        diskLayout->addWidget(diskEdit);
        
        // Other fields
        QLineEdit *hostnameEdit = new QLineEdit(hostname, &dialog);
        QLineEdit *timezoneEdit = new QLineEdit(timezone, &dialog);
        QLineEdit *keymapEdit = new QLineEdit(keymap, &dialog);
        QLineEdit *userEdit = new QLineEdit(userName, &dialog);
        QLineEdit *userPassEdit = new QLineEdit(userPassword, &dialog);
        userPassEdit->setEchoMode(QLineEdit::Password);
        QLineEdit *rootPassEdit = new QLineEdit(rootPassword, &dialog);
        rootPassEdit->setEchoMode(QLineEdit::Password);
        
        // Desktop environment
        QComboBox *desktopCombo = new QComboBox(&dialog);
        desktopCombo->addItem("KDE Plasma", "KDE Plasma");
        desktopCombo->addItem("GNOME", "GNOME");
        desktopCombo->addItem("XFCE", "XFCE");
        desktopCombo->addItem("MATE", "MATE");
        desktopCombo->addItem("LXQt", "LXQt");
        desktopCombo->addItem("None", "None");
        desktopCombo->setCurrentIndex(desktopCombo->findData(desktopEnv));
        
        // Bootloader
        QComboBox *bootloaderCombo = new QComboBox(&dialog);
        bootloaderCombo->addItem("GRUB", "GRUB");
        bootloaderCombo->addItem("rEFInd", "rEFInd");
        bootloaderCombo->setCurrentIndex(bootloaderCombo->findData(bootloader));
        
        // Init system
        QComboBox *initCombo = new QComboBox(&dialog);
        initCombo->addItem("OpenRC", "OpenRC");
        initCombo->addItem("sysvinit", "sysvinit");
        initCombo->addItem("runit", "runit");
        initCombo->addItem("s6", "s6");
        initCombo->setCurrentIndex(initCombo->findData(initSystem));
        
        // Compression level
        QSpinBox *compressionSpin = new QSpinBox(&dialog);
        compressionSpin->setRange(1, 22);
        compressionSpin->setValue(compressionLevel);
        
        // Form layout
        QFormLayout *formLayout = new QFormLayout();
        formLayout->addRow("Target Disk:", diskLayout);
        formLayout->addRow("Hostname:", hostnameEdit);
        formLayout->addRow("Timezone:", timezoneEdit);
        formLayout->addRow("Keymap:", keymapEdit);
        formLayout->addRow("Username:", userEdit);
        formLayout->addRow("User Password:", userPassEdit);
        formLayout->addRow("Root Password:", rootPassEdit);
        formLayout->addRow("Desktop Environment:", desktopCombo);
        formLayout->addRow("Bootloader:", bootloaderCombo);
        formLayout->addRow("Init System:", initCombo);
        formLayout->addRow("BTRFS Compression Level (1-22):", compressionSpin);
        
        scrollLayout->addLayout(formLayout);
        scrollArea->setWidget(scrollContent);
        scrollArea->setWidgetResizable(true);
        
        // Buttons
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        
        layout->addWidget(scrollArea);
        layout->addWidget(buttonBox);
        
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        
        if (dialog.exec() == QDialog::Accepted) {
            targetDisk = diskEdit->text();
            hostname = hostnameEdit->text();
            timezone = timezoneEdit->text();
            keymap = keymapEdit->text();
            userName = userEdit->text();
            userPassword = userPassEdit->text();
            rootPassword = rootPassEdit->text();
            desktopEnv = desktopCombo->currentData().toString();
            bootloader = bootloaderCombo->currentData().toString();
            initSystem = initCombo->currentData().toString();
            compressionLevel = compressionSpin->value();
            
            appendOutput("Installation configuration saved.");
        }
    }
    
    void findFastestMirrors() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Fastest Mirrors", 
                                     "Would you like to find and use the fastest mirrors?",
                                     QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            appendOutput("Finding fastest mirrors...");
            progressBar->setValue(10);
            
            // Run the actual command from the script
            runCommand("apk add reflector");
            runCommand("reflector --latest 20 --protocol https --sort rate --save /etc/apk/repositories");
            
            appendOutput("Mirrorlist updated with fastest mirrors");
            progressBar->setValue(100);
            QTimer::singleShot(1000, [this]() { progressBar->setValue(0); });
        } else {
            appendOutput("Using default mirrors");
        }
    }
    
    void startInstallation() {
        if (targetDisk.isEmpty()) {
            QMessageBox::warning(this, "Error", "Please configure installation first!");
            return;
        }
        
        QString message = QString("About to install to %1 with these settings:\n\n"
                               "Hostname: %2\n"
                               "Timezone: %3\n"
                               "Keymap: %4\n"
                               "Username: %5\n"
                               "Desktop: %6\n"
                               "Bootloader: %7\n"
                               "Init System: %8\n"
                               "Compression Level: %9\n\n"
                               "Continue?").arg(targetDisk, hostname, timezone, keymap, userName, 
                                               desktopEnv, bootloader, initSystem, QString::number(compressionLevel));
        
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Installation", message,
                                    QMessageBox::Yes | QMessageBox::No);
        
        if (reply != QMessageBox::Yes) {
            appendOutput("Installation cancelled.");
            return;
        }
        
        // Start installation process
        appendOutput("=== Starting Alpine Linux BTRFS Installation ===");
        
        // Check if running as root
        if (QProcess::execute("id", {"-u"}) != 0) {
            appendOutput("ERROR: This script must be run as root or with sudo");
            return;
        }
        
        // Check UEFI boot mode
        if (!QFile::exists("/sys/firmware/efi")) {
            appendOutput("ERROR: This script requires UEFI boot mode");
            return;
        }
        
        // Create installation steps
        QStringList steps = {
            "Installing required tools",
            "Partitioning disk",
            "Formatting partitions",
            "Creating BTRFS subvolumes",
            "Mounting filesystems",
            "Installing base system",
            "Configuring chroot environment",
            "Setting up bootloader",
            "Configuring init system",
            "Finalizing installation"
        };
        
        // Execute installation steps
        int step = 0;
        for (const QString &stepDesc : steps) {
            appendOutput(QString("\n>>> %1").arg(stepDesc));
            progressBar->setValue((++step * 100) / steps.count());
            
            if (stepDesc == "Installing required tools") {
                runCommand("apk add btrfs-progs parted dosfstools efibootmgr");
                runCommand("modprobe btrfs");
            }
            else if (stepDesc == "Partitioning disk") {
                runCommand(QString("parted -s %1 mklabel gpt").arg(targetDisk));
                runCommand(QString("parted -s %1 mkpart primary 1MiB 513MiB").arg(targetDisk));
                runCommand(QString("parted -s %1 set 1 esp on").arg(targetDisk));
                runCommand(QString("parted -s %1 mkpart primary 513MiB 100%").arg(targetDisk));
            }
            else if (stepDesc == "Formatting partitions") {
                runCommand(QString("mkfs.vfat -F32 %11").arg(targetDisk));
                runCommand(QString("mkfs.btrfs -f %12").arg(targetDisk));
            }
            else if (stepDesc == "Creating BTRFS subvolumes") {
                runCommand(QString("mount %12 /mnt").arg(targetDisk));
                runCommand("btrfs subvolume create /mnt/@");
                runCommand("btrfs subvolume create /mnt/@home");
                runCommand("btrfs subvolume create /mnt/@root");
                runCommand("btrfs subvolume create /mnt/@srv");
                runCommand("btrfs subvolume create /mnt/@tmp");
                runCommand("btrfs subvolume create /mnt/@log");
                runCommand("btrfs subvolume create /mnt/@cache");
                runCommand("umount /mnt");
            }
            else if (stepDesc == "Mounting filesystems") {
                runCommand(QString("mount -o subvol=@,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt").arg(compressionLevel).arg(targetDisk));
                runCommand("mkdir -p /mnt/boot/efi");
                runCommand(QString("mount %11 /mnt/boot/efi").arg(targetDisk));
                runCommand("mkdir -p /mnt/home");
                runCommand("mkdir -p /mnt/root");
                runCommand("mkdir -p /mnt/srv");
                runCommand("mkdir -p /mnt/tmp");
                runCommand("mkdir -p /mnt/var/cache");
                runCommand("mkdir -p /mnt/var/log");
                runCommand(QString("mount -o subvol=@home,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/home").arg(compressionLevel).arg(targetDisk));
                runCommand(QString("mount -o subvol=@root,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/root").arg(compressionLevel).arg(targetDisk));
                runCommand(QString("mount -o subvol=@srv,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/srv").arg(compressionLevel).arg(targetDisk));
                runCommand(QString("mount -o subvol=@tmp,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/tmp").arg(compressionLevel).arg(targetDisk));
                runCommand(QString("mount -o subvol=@log,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/var/log").arg(compressionLevel).arg(targetDisk));
                runCommand(QString("mount -o subvol=@cache,compress=zstd:%1,compress-force=zstd:%1 %12 /mnt/var/cache").arg(compressionLevel).arg(targetDisk));
            }
            else if (stepDesc == "Installing base system") {
                runCommand("setup-disk -m sys /mnt");
            }
            else if (stepDesc == "Configuring chroot environment") {
                runCommand("mount -t proc none /mnt/proc");
                runCommand("mount --rbind /dev /mnt/dev");
                runCommand("mount --rbind /sys /mnt/sys");
                
                // Create chroot setup script
                QString chrootScript = QString(
                    "#!/bin/ash\n"
                    "echo \"root:%1\" | chpasswd\n"
                    "adduser -D %2 -G wheel,video,audio,input\n"
                    "echo \"%2:%3\" | chpasswd\n"
                    "setup-timezone -z %4\n"
                    "setup-keymap %5 %5\n"
                    "echo \"%6\" > /etc/hostname\n"
                    "cat << EOF > /etc/fstab\n"
                    "%71 /boot/efi vfat defaults 0 2\n"
                    "%72 / btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@ 0 1\n"
                    "%72 /home btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@home 0 2\n"
                    "%72 /root btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@root 0 2\n"
                    "%72 /srv btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@srv 0 2\n"
                    "%72 /tmp btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@tmp 0 2\n"
                    "%72 /var/log btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@log 0 2\n"
                    "%72 /var/cache btrfs rw,noatime,compress=zstd:%8,compress-force=zstd:%8,subvol=@cache 0 2\n"
                    "EOF\n"
                    "apk update\n"
                ).arg(rootPassword, userName, userPassword, timezone, keymap, hostname, targetDisk, QString::number(compressionLevel));
                
                // Add desktop environment setup
                if (desktopEnv == "KDE Plasma") {
                    chrootScript += "setup-desktop plasma\napk add plasma-nm\n";
                } else if (desktopEnv == "GNOME") {
                    chrootScript += "setup-desktop gnome\napk add networkmanager-gnome\n";
                } else if (desktopEnv == "XFCE") {
                    chrootScript += "setup-desktop xfce\napk add networkmanager-gtk\n";
                } else if (desktopEnv == "MATE") {
                    chrootScript += "setup-desktop mate\napk add networkmanager-gtk\n";
                } else if (desktopEnv == "LXQt") {
                    chrootScript += "setup-desktop lxqt\napk add networkmanager-qt\n";
                } else {
                    chrootScript += "apk add networkmanager\n";
                }
                
                // Add bootloader setup
                if (bootloader == "GRUB") {
                    chrootScript += 
                        "apk add grub-efi\n"
                        "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE\n"
                        "grub-mkconfig -o /boot/grub/grub.cfg\n";
                } else if (bootloader == "rEFInd") {
                    chrootScript += "apk add refind\nrefind-install\n";
                }
                
                // Add init system setup
                if (initSystem == "OpenRC") {
                    chrootScript += "rc-update add dbus\nrc-update add networkmanager\n";
                    if (desktopEnv != "None") {
                        QString loginManager = getLoginManager();
                        if (loginManager != "none") {
                            chrootScript += QString("rc-update add %1\n").arg(loginManager);
                        }
                    }
                } else if (initSystem == "sysvinit") {
                    chrootScript += 
                        "apk add sysvinit openrc\n"
                        "ln -sf /etc/inittab.sysvinit /etc/inittab\n";
                    if (desktopEnv != "None") {
                        QString loginManager = getLoginManager();
                        if (loginManager != "none") {
                            chrootScript += QString("ln -s /etc/init.d/%1 /etc/rc.d/\n").arg(loginManager);
                        }
                    }
                } else if (initSystem == "runit") {
                    chrootScript += 
                        "apk add runit runit-openrc\n"
                        "mkdir -p /etc/service\n";
                    if (desktopEnv != "None") {
                        QString loginManager = getLoginManager();
                        if (loginManager != "none") {
                            chrootScript += 
                                QString("mkdir -p /etc/service/%1\n"
                                        "echo '#!/bin/sh' > /etc/service/%1/run\n"
                                        "echo 'exec /etc/init.d/%1 start' >> /etc/service/%1/run\n"
                                        "chmod +x /etc/service/%1/run\n").arg(loginManager);
                        }
                    }
                } else if (initSystem == "s6") {
                    chrootScript += 
                        "apk add s6 s6-openrc\n"
                        "mkdir -p /etc/s6/sv\n";
                    if (desktopEnv != "None") {
                        QString loginManager = getLoginManager();
                        if (loginManager != "none") {
                            chrootScript += 
                                QString("mkdir -p /etc/s6/sv/%1\n"
                                        "echo '#!/bin/sh' > /etc/s6/sv/%1/run\n"
                                        "echo 'exec /etc/init.d/%1 start' >> /etc/s6/sv/%1/run\n"
                                        "chmod +x /etc/s6/sv/%1/run\n").arg(loginManager);
                        }
                    }
                }
                
                chrootScript += "rm /setup-chroot.sh\n";
                
                // Save and execute chroot script
                QFile chrootFile("/mnt/setup-chroot.sh");
                if (chrootFile.open(QIODevice::WriteOnly)) {
                    chrootFile.write(chrootScript.toUtf8());
                    chrootFile.close();
                    runCommand("chmod +x /mnt/setup-chroot.sh");
                    runCommand("chroot /mnt /setup-chroot.sh");
                } else {
                    appendOutput("ERROR: Failed to create chroot setup script");
                }
            }
            else if (stepDesc == "Finalizing installation") {
                runCommand("umount -l /mnt");
                appendOutput("=== Installation complete! ===");
                showPostInstallOptions();
            }
            
            QEventLoop loop;
            QTimer::singleShot(500, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }
    
    void showPostInstallOptions() {
        QDialog dialog(this);
        dialog.setWindowTitle("Installation Complete");
        
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->addWidget(new QLabel("Select post-install action:"));
        
        QPushButton *rebootButton = new QPushButton("Reboot now", &dialog);
        QPushButton *chrootButton = new QPushButton("Chroot into installed system", &dialog);
        QPushButton *exitButton = new QPushButton("Exit without rebooting", &dialog);
        
        layout->addWidget(rebootButton);
        layout->addWidget(chrootButton);
        layout->addWidget(exitButton);
        
        connect(rebootButton, &QPushButton::clicked, [this, &dialog]() {
            appendOutput("Rebooting system...");
            runCommand("reboot");
            dialog.accept();
        });
        
        connect(chrootButton, &QPushButton::clicked, [this, &dialog]() {
            appendOutput("Entering chroot...");
            runCommand(QString("mount %11 /mnt/boot/efi").arg(targetDisk));
            runCommand(QString("mount -o subvol=@ %12 /mnt").arg(targetDisk));
            runCommand("mount -t proc none /mnt/proc");
            runCommand("mount --rbind /dev /mnt/dev");
            runCommand("mount --rbind /sys /mnt/sys");
            runCommand("mount --rbind /dev/pts /mnt/dev/pts");
            runCommand("chroot /mnt /bin/ash");
            runCommand("umount -l /mnt");
            dialog.accept();
        });
        
        connect(exitButton, &QPushButton::clicked, &dialog, &QDialog::accept);
        
        dialog.exec();
    }
    
    void appendOutput(const QString &text) {
        outputConsole->append(text);
        QTextCursor cursor = outputConsole->textCursor();
        cursor.movePosition(QTextCursor::End);
        outputConsole->setTextCursor(cursor);
    }
    
    void showAsciiArt() {
        QString art = R"(
░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗
██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝
██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░
██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗
╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝
░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░
)";
        asciiArtLabel->setText(art);
    }

private:
    void initVariables() {
        // Initialize with empty values (no defaults)
        targetDisk = "";
        hostname = "";
        timezone = "";
        keymap = "";
        userName = "";
        userPassword = "";
        rootPassword = "";
        desktopEnv = "";
        bootloader = "";
        initSystem = "";
        compressionLevel = 0;
    }
    
    QString getLoginManager() {
        if (desktopEnv == "KDE Plasma") return "sddm";
        if (desktopEnv == "GNOME") return "gdm";
        if (desktopEnv == "XFCE" || desktopEnv == "MATE" || desktopEnv == "LXQt") return "lightdm";
        return "none";
    }
    
    void runCommand(const QString &command) {
        appendOutput(QString("$ %1").arg(command));
        
        QProcess process;
        process.start("sh", QStringList() << "-c" << command);
        process.waitForFinished();
        
        QString output = process.readAllStandardOutput();
        QString error = process.readAllStandardError();
        
        if (!output.isEmpty()) {
            appendOutput(output);
        }
        if (!error.isEmpty()) {
            appendOutput(error);
        }
    }
    
    // UI elements
    QLabel *asciiArtLabel;
    QLabel *titleLabel;
    QProgressBar *progressBar;
    QTextEdit *outputConsole;
    QPushButton *configureButton;
    QPushButton *mirrorsButton;
    QPushButton *installButton;
    QPushButton *exitButton;
    
    // Configuration variables
    QString targetDisk;
    QString hostname;
    QString timezone;
    QString keymap;
    QString userName;
    QString userPassword;
    QString rootPassword;
    QString desktopEnv;
    QString bootloader;
    QString initSystem;
    int compressionLevel;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application style
    app.setStyle("Fusion");
    
    // Create and show main window
    AlpineInstaller installer;
    installer.show();
    
    return app.exec();
}

#include "main.moc"
