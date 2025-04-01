## Plasma Login

> [!important]
> [plasma-login](https://invent.kde.org/davidedmundson/plasma-login) and [plasma-login-manager](https://invent.kde.org/davidedmundson/plasma-login-manager) are in a prototype state and are not considered ready for real-world usage.

Plasma Login Manager provides the backend for Plasma's login experience. It is a fork of [SDDM](https://github.com/sddm/sddm).

It requires [plasma-login](https://invent.kde.org/davidedmundson/plasma-login) which provides the frontend.
 
### Getting started

To try Plasma Login, you can build both repositories and install them on your system.

> [!caution]
> It is not recommended to install this on your system — you should use a virtual machine instead. Installing this on real hardware will leave behind files not trivially uninstallable and could leave your system in a non-function state.

You will need to:

- On Arch Linux, install `base-devel`, `git`, `cmake` and `extra-cmake-modules`
- Clone git repositories:

```bash
git clone https://invent.kde.org/davidedmundson/plasma-login.git
git clone https://invent.kde.org/davidedmundson/plasma-login-manager.git
```

- Build and install both repositories:

```bash
cmake -S plasma-login -B plasma-login/build && sudo make install -C plasma-login/build
cmake -S plasma-login-manager -B plasma-login-manager/build && sudo make install -C plasma-login-manager/build
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
