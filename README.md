# Raider <img align="right" alt="Number of downloads" src="https://img.shields.io/flathub/downloads/com.github.ADBeveridge.Raider?style=flat-square"> <img  align="right" alt="AUR" src="https://img.shields.io/aur/version/raider-file-shredder?style=flat-square"></a> <img align="right" alt="Fedora COPR build status" src="https://copr.fedorainfracloud.org/coprs/0xmrtt/raider/package/raider/status_image/last_build.png"/>

Raider is a shredding program built for the GNOME desktop. It uses its own internal shredding code instead of an external program.

## Download
<a href='https://beta.flathub.org/apps/details/com.github.ADBeveridge.Raider'><img width='240' alt='Download on Flathub' src='https://flathub.org/assets/badges/flathub-badge-en.png'/></a>

# Compiling
## GNOME Builder
* Install GNOME Builder. This should be in your distro's repositories. It is also available here: https://flathub.org/apps/org.gnome.Builder
* Download the source of this project from GitHub.
* Open the project in Builder, and after installing SDKs, click the Run command.
## Commandline
### Fedora
* `sudo dnf install meson gcc gtk4-devel libadwaita-devel desktop-file-utils python3-gobject`
* `meson setup builddir`
* `cd builddir`
* `ninja`
* `sudo meson install`

## Useful Links
*   GNOME Circle: <https://apps.gnome.org/app/com.github.ADBeveridge.Raider/>
*   Arch Linux AUR: <https://aur.archlinux.org/packages/raider-file-shredder>
*   Fedora COPR: <https://copr.fedorainfracloud.org/coprs/0xmrtt/raider/package/raider/>
*   Build manifest: <https://github.com/flathub/com.github.ADBeveridge.Raider>
*   Report issues: <https://github.com/ADBeveridge/raider/issues/>
