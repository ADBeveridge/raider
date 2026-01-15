# Building
## GNOME Builder
* Install GNOME Builder. This should be in your distro's repositories. It is also available on Flathub.
* Download the source of this project from GitHub.
* Open the project in Builder, and after installing SDKs if needed, click the Run command.


## Commandline
### Fedora 49
Fedora's default DNF repositories already provide the latest versions of libraries used by Raider. To avoid using the Meson subprojects, run the following:
```
sudo dnf install gtk4-devel libadwaita-devel blueprint-compiler libcurl-devel sassc 
```
After that, you can run the standard set of meson commands to build a program.
```
meson setup builddir
cd builddir
ninja
```

