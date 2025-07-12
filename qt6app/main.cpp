#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QPalette>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QStyleFactory>
#include <QTimer>
#include <QFontDatabase>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QSpinBox>
#include <QDateTime>

class PasswordDialog : public QDialog {
public:
    PasswordDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Enter Password");
        QFormLayout *layout = new QFormLayout(this);

        passwordEdit = new QLineEdit;
        passwordEdit->setEchoMode(QLineEdit::Password);
        confirmEdit = new QLineEdit;
        confirmEdit->setEchoMode(QLineEdit::Password);

        layout->addRow("Password:", passwordEdit);
        layout->addRow("Confirm Password:", confirmEdit);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &PasswordDialog::validate);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        layout->addWidget(buttonBox);
    }

    QString password() const { return passwordEdit->text(); }

private:
    void validate() {
        if (passwordEdit->text() != confirmEdit->text()) {
            QMessageBox::warning(this, "Error", "Passwords do not match!");
            return;
        }
        if (passwordEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Error", "Password cannot be empty!");
            return;
        }
        accept();
    }

    QLineEdit *passwordEdit;
    QLineEdit *confirmEdit;
};

class CommandRunner : public QObject {
    Q_OBJECT
public:
    explicit CommandRunner(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void commandStarted(const QString &command);
    void commandOutput(const QString &output);
    void commandFinished(bool success);

public slots:
    void runCommand(const QString &command, const QStringList &args = QStringList(), bool asRoot = false) {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);

        QString fullCommand;
        if (asRoot) {
            fullCommand = "doas " + command;
            if (!args.isEmpty()) {
                fullCommand += " " + args.join(" ");
            }
        } else {
            fullCommand = command;
            if (!args.isEmpty()) {
                fullCommand += " " + args.join(" ");
            }
        }

        emit commandStarted(fullCommand);

        if (asRoot) {
            QStringList fullArgs;
            fullArgs << command;
            fullArgs += args;

            process.start("doas", fullArgs);
            if (!process.waitForStarted()) {
                emit commandOutput("Failed to start doas command\n");
                emit commandFinished(false);
                return;
            }

            process.write((m_sudoPassword + "\n").toUtf8());
            process.closeWriteChannel();
        } else {
            process.start(command, args);
        }

        if (!process.waitForStarted()) {
            emit commandOutput("Failed to start command: " + fullCommand + "\n");
            emit commandFinished(false);
            return;
        }

        while (process.waitForReadyRead()) {
            QByteArray output = process.readAll();
            emit commandOutput(QString::fromUtf8(output));
        }

        process.waitForFinished();

        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
            emit commandFinished(true);
        } else {
            emit commandOutput(QString("Command failed with exit code %1\n").arg(process.exitCode()));
            emit commandFinished(false);
        }
    }

    void setSudoPassword(const QString &password) {
        m_sudoPassword = password;
        m_sudoPassword.replace("'", "'\\''");
    }

private:
    QString m_sudoPassword;
};

class AlpineInstaller : public QMainWindow {
    Q_OBJECT

public:
    AlpineInstaller(QWidget *parent = nullptr) : QMainWindow(parent) {
        // Set dark theme
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53,53,53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25,25,25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53,53,53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);

        // Create main widgets
        QWidget *centralWidget = new QWidget;
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

        // ASCII art title
        QLabel *titleLabel = new QLabel;
        titleLabel->setText(
            "<span style='font-size:6px; color:#ff0000;'>░█████╗░██╗░░░░░░█████╗░██║░░░██╗██████╗░███████╗███╗░░░███╗░█████╗░██████╗░░██████╗<br>"
            "██╔══██╗██║░░░░░██╔══██╗██║░░░██║██╔══██╗██╔════╝████╗░████║██╔══██╗██╔══██╗██╔════╝<br>"
            "██║░░╚═╝██║░░░░░███████║██║░░░██║██║░░██║█████╗░░██╔████╔██║██║░░██║██║░░██║╚█████╗░<br>"
            "██║░░██╗██║░░░░░██╔══██║██║░░░██║██║░░██║██╔══╝░░██║╚██╔╝██║██║░░██║██║░░██║░╚═══██╗<br>"
            "╚█████╔╝███████╗██║░░██║╚██████╔╝██████╔╝███████╗██║░╚═╝░██║╚█████╔╝██████╔╝██████╔╝<br>"
            "░╚════╝░╚══════╝╚═╝░░╚═╝░╚═════╝░╚═════╝░╚══════╝╚═╝░░░░░╚═╝░╚════╝░╚═════╝░╚═════╝░</span><br>"
            "<span style='color:#00ffff;'>Alpine Btrfs Installer v1.0 12-07-2025</span>"
        );
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);

        // Progress bar with log button
        QHBoxLayout *progressLayout = new QHBoxLayout;
        progressBar = new QProgressBar;
        progressBar->setRange(0, 100);
        progressBar->setTextVisible(true);

        // Customize progress bar with cyan gradient
        QString progressStyle =
        "QProgressBar {"
        "    border: 2px solid grey;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "    background: #252525;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "        stop:0 #00ffff, stop:1 #00aaff);"
        "}";
        progressBar->setStyleSheet(progressStyle);

        QPushButton *logButton = new QPushButton("Log");
        logButton->setFixedWidth(60);
        connect(logButton, &QPushButton::clicked, this, &AlpineInstaller::toggleLog);

        progressLayout->addWidget(progressBar, 1);
        progressLayout->addWidget(logButton);
        mainLayout->addLayout(progressLayout);

        // Log area
        logArea = new QTextEdit;
        logArea->setReadOnly(true);
        logArea->setFont(QFont("Monospace", 10));
        logArea->setStyleSheet("background-color: #252525; color: #00ffff;");
        logArea->setVisible(false);
        mainLayout->addWidget(logArea);

        // Buttons
        QHBoxLayout *buttonLayout = new QHBoxLayout;
        QPushButton *configButton = new QPushButton("Configure Installation");
        QPushButton *mirrorButton = new QPushButton("Find Fastest Mirrors");
        QPushButton *installButton = new QPushButton("Start Installation");
        QPushButton *exitButton = new QPushButton("Exit");

        connect(configButton, &QPushButton::clicked, this, &AlpineInstaller::configureInstallation);
        connect(mirrorButton, &QPushButton::clicked, this, &AlpineInstaller::findFastestMirrors);
        connect(installButton, &QPushButton::clicked, this, &AlpineInstaller::startInstallation);
        connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);

        buttonLayout->addWidget(configButton);
        buttonLayout->addWidget(mirrorButton);
        buttonLayout->addWidget(installButton);
        buttonLayout->addWidget(exitButton);
        mainLayout->addLayout(buttonLayout);

        setCentralWidget(centralWidget);
        setWindowTitle("Alpine Linux BTRFS Installer");
        resize(800, 600);

        // Initialize empty settings
        initSettings();

        // Setup command runner thread
        commandThread = new QThread;
        commandRunner = new CommandRunner;
        commandRunner->moveToThread(commandThread);

        connect(this, &AlpineInstaller::executeCommand, commandRunner, &CommandRunner::runCommand);
        connect(commandRunner, &CommandRunner::commandStarted, this, &AlpineInstaller::logCommand);
        connect(commandRunner, &CommandRunner::commandOutput, this, &AlpineInstaller::logOutput);
        connect(commandRunner, &CommandRunner::commandFinished, this, &AlpineInstaller::commandCompleted);

        commandThread->start();
    }

    ~AlpineInstaller() {
        commandThread->quit();
        commandThread->wait();
        delete commandRunner;
        delete commandThread;
    }

signals:
    void executeCommand(const QString &command, const QStringList &args = QStringList(), bool asRoot = false);

private slots:
    void toggleLog() {
        logArea->setVisible(!logArea->isVisible());
    }

    void configureInstallation() {
        QDialog dialog(this);
        dialog.setWindowTitle("Configure Installation");

        QFormLayout *form = new QFormLayout(&dialog);

        // Disk selection
        QLineEdit *diskEdit = new QLineEdit(settings["targetDisk"]);
        diskEdit->setPlaceholderText("e.g. /dev/sda");
        form->addRow("Target Disk:", diskEdit);

        // Hostname
        QLineEdit *hostnameEdit = new QLineEdit(settings["hostname"]);
        hostnameEdit->setPlaceholderText("e.g. alpine");
        form->addRow("Hostname:", hostnameEdit);

        // Timezone
        QLineEdit *timezoneEdit = new QLineEdit(settings["timezone"]);
        timezoneEdit->setPlaceholderText("e.g. UTC");
        form->addRow("Timezone:", timezoneEdit);

        // Keymap
        QLineEdit *keymapEdit = new QLineEdit(settings["keymap"]);
        keymapEdit->setPlaceholderText("e.g. us");
        form->addRow("Keymap:", keymapEdit);

        // Username
        QLineEdit *userEdit = new QLineEdit(settings["username"]);
        userEdit->setPlaceholderText("e.g. user");
        form->addRow("Username:", userEdit);

        // Desktop environment
        QComboBox *desktopCombo = new QComboBox;
        desktopCombo->addItem("", ""); // Empty default
        desktopCombo->addItem("KDE Plasma", "KDE Plasma");
        desktopCombo->addItem("GNOME", "GNOME");
        desktopCombo->addItem("XFCE", "XFCE");
        desktopCombo->addItem("MATE", "MATE");
        desktopCombo->addItem("LXQt", "LXQt");
        desktopCombo->addItem("None", "None");
        if (!settings["desktopEnv"].isEmpty()) {
            desktopCombo->setCurrentText(settings["desktopEnv"]);
        }
        form->addRow("Desktop Environment:", desktopCombo);

        // Bootloader
        QComboBox *bootloaderCombo = new QComboBox;
        bootloaderCombo->addItem("", ""); // Empty default
        bootloaderCombo->addItem("GRUB", "GRUB");
        bootloaderCombo->addItem("rEFInd", "rEFInd");
        if (!settings["bootloader"].isEmpty()) {
            bootloaderCombo->setCurrentText(settings["bootloader"]);
        }
        form->addRow("Bootloader:", bootloaderCombo);

        // Init system
        QComboBox *initCombo = new QComboBox;
        initCombo->addItem("", ""); // Empty default
        initCombo->addItem("OpenRC", "OpenRC");
        initCombo->addItem("sysvinit", "sysvinit");
        initCombo->addItem("runit", "runit");
        initCombo->addItem("s6", "s6");
        if (!settings["initSystem"].isEmpty()) {
            initCombo->setCurrentText(settings["initSystem"]);
        }
        form->addRow("Init System:", initCombo);

        // Compression level
        QSpinBox *compressionSpin = new QSpinBox;
        compressionSpin->setRange(1, 22);
        if (!settings["compressionLevel"].isEmpty()) {
            compressionSpin->setValue(settings["compressionLevel"].toInt());
        } else {
            compressionSpin->setValue(22);
            compressionSpin->setSpecialValueText("Select (1-22)");
        }
        form->addRow("BTRFS Compression Level:", compressionSpin);

        // Password buttons
        QPushButton *rootPassButton = new QPushButton(settings["rootPassword"].isEmpty() ? "Set Root Password" : "Change Root Password");
        QPushButton *userPassButton = new QPushButton(settings["userPassword"].isEmpty() ? "Set User Password" : "Change User Password");
        form->addRow(rootPassButton);
        form->addRow(userPassButton);

        connect(rootPassButton, &QPushButton::clicked, [this, rootPassButton]() {
            PasswordDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                settings["rootPassword"] = dlg.password();
                rootPassButton->setText("Change Root Password");
            }
        });

        connect(userPassButton, &QPushButton::clicked, [this, userPassButton]() {
            PasswordDialog dlg(this);
            if (dlg.exec() == QDialog::Accepted) {
                settings["userPassword"] = dlg.password();
                userPassButton->setText("Change User Password");
            }
        });

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                   Qt::Horizontal, &dialog);
        form->addRow(&buttonBox);

        connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            settings["targetDisk"] = diskEdit->text();
            settings["hostname"] = hostnameEdit->text();
            settings["timezone"] = timezoneEdit->text();
            settings["keymap"] = keymapEdit->text();
            settings["username"] = userEdit->text();
            settings["desktopEnv"] = desktopCombo->currentText();
            settings["bootloader"] = bootloaderCombo->currentText();
            settings["initSystem"] = initCombo->currentText();
            settings["compressionLevel"] = QString::number(compressionSpin->value());

            logMessage("Installation configured with the following settings:");
            logMessage(QString("Target Disk: %1").arg(settings["targetDisk"]));
            logMessage(QString("Hostname: %1").arg(settings["hostname"]));
            logMessage(QString("Timezone: %1").arg(settings["timezone"]));
            logMessage(QString("Keymap: %1").arg(settings["keymap"]));
            logMessage(QString("Username: %1").arg(settings["username"]));
            logMessage(QString("Desktop Environment: %1").arg(settings["desktopEnv"]));
            logMessage(QString("Bootloader: %1").arg(settings["bootloader"]));
            logMessage(QString("Init System: %1").arg(settings["initSystem"]));
            logMessage(QString("Compression Level: %1").arg(settings["compressionLevel"]));
        }
    }

    void findFastestMirrors() {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Fastest Mirrors",
                                      "Would you like to find and use the fastest mirrors?",
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            logMessage("Finding fastest mirrors...");
            progressBar->setValue(10);

            // Install reflector if not present
            emit executeCommand("apk", {"add", "reflector"}, true);

            // Find and update mirrors
            emit executeCommand("reflector", {"--latest", "20", "--protocol", "https", "--sort", "rate", "--save", "/etc/apk/repositories"}, true);

            progressBar->setValue(100);
            QTimer::singleShot(1000, [this]() {
                progressBar->setValue(0);
            });
        } else {
            logMessage("Using default mirrors");
        }
    }

    void startInstallation() {
        // Validate all required settings
        QStringList missingFields;
        if (settings["targetDisk"].isEmpty()) missingFields << "Target Disk";
        if (settings["hostname"].isEmpty()) missingFields << "Hostname";
        if (settings["timezone"].isEmpty()) missingFields << "Timezone";
        if (settings["keymap"].isEmpty()) missingFields << "Keymap";
        if (settings["username"].isEmpty()) missingFields << "Username";
        if (settings["desktopEnv"].isEmpty()) missingFields << "Desktop Environment";
        if (settings["bootloader"].isEmpty()) missingFields << "Bootloader";
        if (settings["initSystem"].isEmpty()) missingFields << "Init System";
        if (settings["compressionLevel"].isEmpty()) missingFields << "Compression Level";
        if (settings["rootPassword"].isEmpty()) missingFields << "Root Password";
        if (settings["userPassword"].isEmpty()) missingFields << "User Password";

        if (!missingFields.isEmpty()) {
            QMessageBox::warning(this, "Error",
                                 QString("The following required fields are missing:\n%1\n\nPlease configure all settings before installation.")
                                 .arg(missingFields.join("\n")));
            return;
        }

        // Show confirmation dialog
        QString confirmationText = QString(
            "About to install to %1 with these settings:\n"
            "Hostname: %2\n"
            "Timezone: %3\n"
            "Keymap: %4\n"
            "Username: %5\n"
            "Desktop: %6\n"
            "Bootloader: %7\n"
            "Init System: %8\n"
            "Compression Level: %9\n\n"
            "Continue?"
        ).arg(
            settings["targetDisk"],
            settings["hostname"],
            settings["timezone"],
            settings["keymap"],
            settings["username"],
            settings["desktopEnv"],
            settings["bootloader"],
            settings["initSystem"],
            settings["compressionLevel"]
        );

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Installation", confirmationText,
                                      QMessageBox::Yes|QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            logMessage("Installation cancelled.");
            return;
        }

        // Ask for sudo password before proceeding
        PasswordDialog passDialog(this);
        passDialog.setWindowTitle("Enter doas Password");
        if (passDialog.exec() != QDialog::Accepted) {
            logMessage("Installation cancelled - no password provided.");
            return;
        }

        // Set sudo password for command runner
        commandRunner->setSudoPassword(passDialog.password());

        // Start installation process
        logMessage("Starting Alpine Linux BTRFS installation...");
        progressBar->setValue(5);

        // Begin installation steps
        currentStep = 0;
        totalSteps = 15; // Total number of installation steps
        nextInstallationStep();
    }

    void nextInstallationStep() {
        currentStep++;
        int progress = (currentStep * 100) / totalSteps;
        progressBar->setValue(progress);

        QString disk1 = settings["targetDisk"] + "1";
        QString disk2 = settings["targetDisk"] + "2";
        QString compression = "zstd:" + settings["compressionLevel"];

        switch (currentStep) {
            case 1: // Install required tools
                logMessage("Installing required tools...");
                emit executeCommand("apk", {"add", "btrfs-progs", "parted", "dosfstools", "efibootmgr"}, true);
                break;

            case 2: // Load btrfs module
                logMessage("Loading BTRFS module...");
                emit executeCommand("modprobe", {"btrfs"}, true);
                break;

            case 3: // Partitioning
                logMessage("Partitioning disk...");
                emit executeCommand("parted", {"-s", settings["targetDisk"], "mklabel", "gpt"}, true);
                emit executeCommand("parted", {"-s", settings["targetDisk"], "mkpart", "primary", "1MiB", "513MiB"}, true);
                emit executeCommand("parted", {"-s", settings["targetDisk"], "set", "1", "esp", "on"}, true);
                emit executeCommand("parted", {"-s", settings["targetDisk"], "mkpart", "primary", "513MiB", "100%"}, true);
                break;

            case 4: // Formatting
                logMessage("Formatting partitions...");
                emit executeCommand("mkfs.vfat", {"-F32", disk1}, true);
                emit executeCommand("mkfs.btrfs", {"-f", disk2}, true);
                break;

            case 5: // Mounting and subvolumes
                logMessage("Creating BTRFS subvolumes...");
                emit executeCommand("mount", {disk2, "/mnt"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@home"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@root"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@srv"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@tmp"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@log"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@cache"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@/var/lib/portables"}, true);
                emit executeCommand("btrfs", {"subvolume", "create", "/mnt/@/var/lib/machines"}, true);
                emit executeCommand("umount", {"/mnt"}, true);
                break;

            case 6: // Remount with compression
                logMessage("Remounting with compression...");
                emit executeCommand("mount", {"-o", QString("subvol=@,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/boot/efi"}, true);
                emit executeCommand("mount", {disk1, "/mnt/boot/efi"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/home"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/root"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/srv"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/tmp"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/cache"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/log"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/lib/portables"}, true);
                emit executeCommand("mkdir", {"-p", "/mnt/var/lib/machines"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@home,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/home"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@root,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/root"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@srv,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/srv"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@tmp,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/tmp"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@log,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/log"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@cache,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/cache"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@/var/lib/portables,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/lib/portables"}, true);
                emit executeCommand("mount", {"-o", QString("subvol=@/var/lib/machines,compress=%1,compress-force=%1").arg(compression), disk2, "/mnt/var/lib/machines"}, true);
                break;

            case 7: // Install base system
                logMessage("Installing base system...");
                emit executeCommand("setup-disk", {"-m", "sys", "/mnt"}, true);
                break;

            case 8: // Mount required filesystems for chroot
                logMessage("Preparing chroot environment...");
                emit executeCommand("mount", {"-t", "proc", "none", "/mnt/proc"}, true);
                emit executeCommand("mount", {"--rbind", "/dev", "/mnt/dev"}, true);
                emit executeCommand("mount", {"--rbind", "/sys", "/mnt/sys"}, true);
                break;

            case 9: // Create chroot setup script
                logMessage("Preparing chroot setup script...");
                createChrootScript();
                emit executeCommand("chmod", {"+x", "/mnt/setup-chroot.sh"}, true);
                break;

            case 10: // Run chroot setup
                logMessage("Running chroot setup...");
                emit executeCommand("chroot", {"/mnt", "/setup-chroot.sh"}, true);
                break;

            case 11: // Clean up
                logMessage("Cleaning up...");
                emit executeCommand("umount", {"-R", "/mnt"}, true);
                break;

            case 12: // Installation complete
                logMessage("Installation complete!");
                progressBar->setValue(100);
                showPostInstallOptions();
                break;

            default:
                break;
        }
    }

    void commandCompleted(bool success) {
        if (!success) {
            logMessage("ERROR: Command failed!");
            QMessageBox::critical(this, "Error", "A command failed during installation. Check the log for details.");
            progressBar->setValue(0);
            return;
        }

        // Proceed to next step
        QTimer::singleShot(500, this, &AlpineInstaller::nextInstallationStep);
    }

    void createChrootScript() {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            QTextStream out(&tempFile);

            out << "#!/bin/ash\n\n";
            out << "# Basic system configuration\n";
            out << "echo \"root:" << settings["rootPassword"] << "\" | chpasswd\n";
            out << "adduser -D " << settings["username"] << " -G wheel,video,audio,input\n";
            out << "echo \"" << settings["username"] << ":" << settings["userPassword"] << "\" | chpasswd\n";
            out << "setup-timezone -z " << settings["timezone"] << "\n";
            out << "setup-keymap " << settings["keymap"] << " " << settings["keymap"] << "\n";
            out << "echo \"" << settings["hostname"] << "\" > /etc/hostname\n\n";

            // Generate fstab
            QString disk1 = settings["targetDisk"] + "1";
            QString disk2 = settings["targetDisk"] + "2";
            QString compression = "zstd:" + settings["compressionLevel"];

            out << "cat << EOF > /etc/fstab\n";
            out << disk1 << " /boot/efi vfat defaults 0 2\n";
            out << disk2 << " / btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@ 0 1\n";
            out << disk2 << " /home btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@home 0 2\n";
            out << disk2 << " /root btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@root 0 2\n";
            out << disk2 << " /srv btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@srv 0 2\n";
            out << disk2 << " /tmp btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@tmp 0 2\n";
            out << disk2 << " /var/log btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@log 0 2\n";
            out << disk2 << " /var/cache btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@cache 0 2\n";
            out << disk2 << " /var/lib/portables btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@/var/lib/portables 0 2\n";
            out << disk2 << " /var/lib/machines btrfs rw,noatime,compress=" << compression << ",compress-force=" << compression << ",subvol=@/var/lib/machines 0 2\n";
            out << "EOF\n\n";

            // Update repositories and install desktop environment
            out << "apk update\n";

            QString loginManager = "none";
            if (settings["desktopEnv"] == "KDE Plasma") {
                out << "setup-desktop plasma\n";
                out << "apk add plasma-nm\n";
                loginManager = "sddm";
            } else if (settings["desktopEnv"] == "GNOME") {
                out << "setup-desktop gnome\n";
                out << "apk add networkmanager-gnome\n";
                loginManager = "gdm";
            } else if (settings["desktopEnv"] == "XFCE") {
                out << "setup-desktop xfce\n";
                out << "apk add networkmanager-gtk\n";
                loginManager = "lightdm";
            } else if (settings["desktopEnv"] == "MATE") {
                out << "setup-desktop mate\n";
                out << "apk add networkmanager-gtk\n";
                loginManager = "lightdm";
            } else if (settings["desktopEnv"] == "LXQt") {
                out << "setup-desktop lxqt\n";
                out << "apk add networkmanager-qt\n";
                loginManager = "lightdm";
            } else {
                out << "echo \"No desktop environment selected\"\n";
                out << "apk add networkmanager\n";
            }

            // Install bootloader
            if (settings["bootloader"] == "GRUB") {
                out << "apk add grub-efi\n";
                out << "grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=ALPINE\n";
                out << "grub-mkconfig -o /boot/grub/grub.cfg\n";
            } else if (settings["bootloader"] == "rEFInd") {
                out << "apk add refind\n";
                out << "refind-install\n";
            }

            // Configure init system
            if (settings["initSystem"] == "OpenRC") {
                out << "rc-update add dbus\n";
                out << "rc-update add networkmanager\n";
                if (loginManager != "none") {
                    out << "rc-update add " << loginManager << "\n";
                }
            } else if (settings["initSystem"] == "sysvinit") {
                out << "apk add sysvinit openrc\n";
                out << "ln -sf /etc/inittab.sysvinit /etc/inittab\n";
                if (loginManager != "none") {
                    out << "ln -s /etc/init.d/" << loginManager << " /etc/rc.d/\n";
                }
                out << "ln -s /etc/init.d/dbus /etc/rc.d/\n";
                out << "ln -s /etc/init.d/networkmanager /etc/rc.d/\n";
            } else if (settings["initSystem"] == "runit") {
                out << "apk add runit runit-openrc\n";
                out << "mkdir -p /etc/service\n";
                if (loginManager != "none") {
                    out << "mkdir -p /etc/service/" << loginManager << "\n";
                    out << "echo '#!/bin/sh' > /etc/service/" << loginManager << "/run\n";
                    out << "echo 'exec /etc/init.d/" << loginManager << " start' >> /etc/service/" << loginManager << "/run\n";
                    out << "chmod +x /etc/service/" << loginManager << "/run\n";
                }
                out << "mkdir -p /etc/service/dbus\n";
                out << "echo '#!/bin/sh' > /etc/service/dbus/run\n";
                out << "echo 'exec /etc/init.d/dbus start' >> /etc/service/dbus/run\n";
                out << "chmod +x /etc/service/dbus/run\n";
            } else if (settings["initSystem"] == "s6") {
                out << "apk add s6 s6-openrc\n";
                out << "mkdir -p /etc/s6/sv\n";
                if (loginManager != "none") {
                    out << "mkdir -p /etc/s6/sv/" << loginManager << "\n";
                    out << "echo '#!/bin/sh' > /etc/s6/sv/" << loginManager << "/run\n";
                    out << "echo 'exec /etc/init.d/" << loginManager << " start' >> /etc/s6/sv/" << loginManager << "/run\n";
                    out << "chmod +x /etc/s6/sv/" << loginManager << "/run\n";
                }
                out << "mkdir -p /etc/s6/sv/dbus\n";
                out << "echo '#!/bin/sh' > /etc/s6/sv/dbus/run\n";
                out << "echo 'exec /etc/init.d/dbus start' >> /etc/s6/sv/dbus/run\n";
                out << "chmod +x /etc/s6/sv/dbus/run\n";
            }

            // Clean up
            out << "rm /setup-chroot.sh\n";

            tempFile.close();

            // Copy the temporary file to /mnt/setup-chroot.sh
            QProcess::execute("cp", {tempFile.fileName(), "/mnt/setup-chroot.sh"});
        }
    }

    void showPostInstallOptions() {
        QDialog dialog(this);
        dialog.setWindowTitle("Installation Complete");

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        QLabel *label = new QLabel("Installation complete! Select post-install action:");
        layout->addWidget(label);

        QPushButton *rebootButton = new QPushButton("Reboot now");
        QPushButton *chrootButton = new QPushButton("Chroot into installed system");
        QPushButton *exitButton = new QPushButton("Exit without rebooting");

        layout->addWidget(rebootButton);
        layout->addWidget(chrootButton);
        layout->addWidget(exitButton);

        connect(rebootButton, &QPushButton::clicked, [this, &dialog]() {
            QProcess::execute("reboot", QStringList());
            dialog.accept();
        });

        connect(chrootButton, &QPushButton::clicked, [this, &dialog]() {
            logMessage("Entering chroot...");
            QString disk1 = settings["targetDisk"] + "1";
            QString disk2 = settings["targetDisk"] + "2";

            emit executeCommand("mount", {disk1, "/mnt/boot/efi"}, true);
            emit executeCommand("mount", {"-o", "subvol=@", disk2, "/mnt"}, true);
            emit executeCommand("mount", {"-t", "proc", "none", "/mnt/proc"}, true);
            emit executeCommand("mount", {"--rbind", "/dev", "/mnt/dev"}, true);
            emit executeCommand("mount", {"--rbind", "/sys", "/mnt/sys"}, true);
            emit executeCommand("mount", {"--rbind", "/dev/pts", "/mnt/dev/pts"}, true);
            emit executeCommand("chroot", {"/mnt", "/bin/ash"}, true);
            emit executeCommand("umount", {"-l", "/mnt"}, true);

            dialog.accept();
        });

        connect(exitButton, &QPushButton::clicked, &dialog, &QDialog::accept);

        dialog.exec();
    }

    void logMessage(const QString &message) {
        logArea->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), message));
        logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
    }

    void logCommand(const QString &command) {
        logMessage("Executing: " + command);
    }

    void logOutput(const QString &output) {
        logArea->insertPlainText(output);
        logArea->verticalScrollBar()->setValue(logArea->verticalScrollBar()->maximum());
    }

private:
    void initSettings() {
        settings["targetDisk"] = "";
        settings["hostname"] = "";
        settings["timezone"] = "";
        settings["keymap"] = "";
        settings["username"] = "";
        settings["desktopEnv"] = "";
        settings["bootloader"] = "";
        settings["initSystem"] = "";
        settings["compressionLevel"] = "";
        settings["rootPassword"] = "";
        settings["userPassword"] = "";
    }

    QProgressBar *progressBar;
    QTextEdit *logArea;
    QMap<QString, QString> settings;
    int currentStep;
    int totalSteps;
    CommandRunner *commandRunner;
    QThread *commandThread;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Check if running as root
    if (QProcess::execute("whoami", QStringList()) != 0) {
        QMessageBox::critical(nullptr, "Error", "This application must be run as root!");
        return 1;
    }

    AlpineInstaller installer;
    installer.show();

    return app.exec();
}

#include "main.moc"
