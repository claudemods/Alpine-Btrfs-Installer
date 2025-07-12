<p align="center">
  <img src="https://i.postimg.cc/d1VR617H/alpine.webp" width="300">
</p>

<div align="center">
  <a href="https://www.alpinelinux.org/" target="_blank">
    <img src="https://img.shields.io/badge/DISTRO-Alpine-00FFFF?style=for-the-badge&logo=Alpine-Linux">
  </a>
</div>

<div align="center">
  <h1>🏔️ Alpine-Btrfs-Installer</h1>
  <h3>v1.02 - UEFI Btrfs Installer with Multiple Init Systems</h3>
</div>

<div align="center">
  <a href="https://www.deepseek.com/" target="_blank">
    <img alt="Homepage" src="https://i.postimg.cc/Hs2vbbZ8/Deep-Seek-Homepage.png" style="height: 30px; width: auto;">
  </a>
</div>

<div align="center">
  <h2><a href="https://github.com/claudemods/Alpine-Btrfs-Installer/tree/main/Photos">📸 Installation Screenshots</a></h2>
  <h2><a href="https://www.youtube.com/watch?v=nnSCQLa2Hnw">▶️ Video Installation Guide</a></h2>
</div>

<div align="center">
  <h2>✨ Key Features</h2>
  <ul style="text-align: left; display: inline-block;">
    <li>✅ UEFI-only Btrfs installation with zstd compression (1-22)</li>
    <li>🔄 Multiple init system support (OpenRC, sysvinit, runit, s6)</li>
    <li>💻 Desktop environments (KDE Plasma, GNOME, XFCE, MATE, LXQt)</li>
    <li>🔌 Bootloader options (GRUB, rEFInd)</li>
    <li>🛠️ Automatic mirror optimization</li>
    <li>🔐 Secure user setup with password protection</li>
  </ul>
</div>

<div align="center">
  <h2>🗂️ Btrfs Subvolume Layout</h2>
  <pre>
@           - Root filesystem
@home       - User home directories
@root       - Root user home
@srv        - Service data
@tmp        - Temporary files
@log        - System logs
@cache      - Package cache
  </pre>
  <p><em>All subvolumes use zstd compression at your selected level (default: 22)</em></p>
</div>

<div align="center">
  <h2>🚀 Installation Guide</h2>
  
  <h3>Prerequisites</h3>
  <p>Booted into Alpine Linux live environment with internet access</p>
  
  <h3>Quick Start</h3>
  <pre>
git clone https://github.com/claudemods/Alpine-Btrfs-Installer
cd Alpine-Btrfs-Installer
chmod +x *.sh
./installer-gui-using-dialog.sh
  </pre>

  <h3>Installation Steps</h3>
  <ol style="text-align: left; display: inline-block;">
    <li>1️⃣ Configure system settings (hostname, timezone, keymap)</li>
    <li>2️⃣ Set up user and root passwords</li>
    <li>3️⃣ Select target disk for installation</li>
    <li>4️⃣ Choose desktop environment (optional)</li>
    <li>5️⃣ Select init system (OpenRC, sysvinit, runit, or s6)</li>
    <li>6️⃣ Configure bootloader (GRUB or rEFInd)</li>
    <li>7️⃣ Set Btrfs compression level (1-22)</li>
    <li>8️⃣ Confirm and begin installation</li>
  </ol>
</div>

<div align="center">
  <h2>📦 Pre-built Image</h2>
  <p>260MB minimal .img.xz with installer included</p>
  <p><a href="https://drive.google.com/drive/folders/1BdjKB6pDIVhP-sAY5hXs1CyVxmaHnlw7">Download Installer Image</a></p>
</div>

<div align="center">
  <h2>⚙️ Technical Details</h2>
  <ul style="text-align: left; display: inline-block;">
    <li><strong>Kernel:</strong> Alpine Linux LTS</li>
    <li><strong>Filesystem:</strong> Btrfs with zstd compression</li>
    <li><strong>Partitioning:</strong> Automatic UEFI GPT partitioning</li>
    <li><strong>Network:</strong> NetworkManager included with desktop environments</li>
  </ul>
</div>

<div align="center">
  <h2>🔄 Post-Installation</h2>
  <ul style="text-align: left; display: inline-block;">
    <li>Chroot into installed system for additional configuration</li>
    <li>Automatic fstrim timer setup for SSD optimization</li>
    <li>Option to install additional packages in chroot</li>
  </ul>
</div>

<div align="center">
  <h2>🤝 Contributing</h2>
  <p>Feel free to fork or contribute to this project!</p>
  <p>Report issues or suggest improvements on GitHub</p>
</div>
