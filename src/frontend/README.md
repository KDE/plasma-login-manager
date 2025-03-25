## Plasma Login

> [!important]
> [plasma-login](https://invent.kde.org/davidedmundson/plasma-login) and [plasma-login-manager](https://invent.kde.org/davidedmundson/plasma-login-manager) are in a prototype state and are not considered ready for real-world usage.

Plasma Login provides the frontend for Plasma's login experience, including:

 - Login greeter
 - Login wallpaper
 - System Settings module (KCM)

It requires [plasma-login-manager](https://invent.kde.org/davidedmundson/plasma-login-manager) which provides the backend.
 
### Getting started

To try Plasma Login, you can build both repositories and install them on your system.

> [!caution]
> It is not recommended to install this on your system — you should use a virtual machine instead.

You will need to:

- Build and install both, e.g. `cmake -S . -B build -DBUILD_WITH_QT6=ON && sudo make install -C build`
- Disable `sddm` systemd service and enable `plasmalogin`
- Reboot…

…and be prepared to review logs to work around subtle issues.
