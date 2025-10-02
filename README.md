## Plasma Login

> [!important]
> Plasma Login is in a prototype state and is not considered ready for real-world usage.

Plasma Login provides a display manager for KDE Plasma, forked from [SDDM](https://github.com/sddm/sddm) and with an new frontend providing a greeter, wallpaper plugin integration and System Settings module (KCM).

### What we want

 - Great out-of-box experience in multi-monitor and high DPI and HDR
 - Keyboard layout switching
 - Virtual keyboards
 - Easy Chinese/Japanese/Korean/Vietnamese (CJK) input
 - Screen readers for blind people (which then means volume control)
 - Remote (VNC/RDP) support from startup
 - Deeper Plasma integration including:
    - Display and keyboard brightness control
    - Full power management
    - Pairing trusted bluetooth devices
    - Login to known Wi-Fi for remote LDAP

### Getting started

To try Plasma Login, you can build both repositories and install them on your system.

> [!caution]
> It is not recommended to install this on your system — you should use a virtual machine instead. Installing this on real hardware will leave behind files not trivially uninstallable and could leave your system in a non-functional state.

You will need to:

- On Arch Linux, install `base-devel`, `git`, `cmake` and `extra-cmake-modules`
- Clone, build and install:

```bash
git clone https://invent.kde.org/plasma/plasma-login-manager.git
cmake -S plasma-login-manager -B plasma-login-manager/build && sudo make install -C plasma-login-manager/build
```

- Trigger the system user to be created:

```bash
sudo systemd-sysusers
```

- Disable SDDM and enable Plasma Login:

```bash
sudo systemctl disable sddm
sudo systemctl enable plasmalogin
```

- Copy PAM files:

```bash
sudo cp /etc/pam.d/sddm /etc/pam.d/plasmalogin
sudo cp /etc/pam.d/sddm-autologin /etc/pam.d/plasmalogin-autologin
sudo cp /etc/pam.d/sddm-greeter /etc/pam.d/plasmalogin-greeter
```

- …and finally reboot.

### Configuration

Plasma Login is configured by users through `/etc/plasmalogin.conf`, which overrides distro-provided defaults at `/usr/lib/plasma-login/defaults.conf`. In managed scenarios, the latter file can be modified to set a default wallpaper or login session, with the settings module disabled via Kiosk.
