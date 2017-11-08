i3lock-fancier - yet another i3lock fork
========================================

This fork combines features of [i3lock-color](https://github.com/chrjguill/i3lock-color)
and [this i3lock fork] (https://github.com/cac03/i3lock/commits/master) to
create my ultimate lockscreen experience.

Drawbacks:
* I don't know how to work with OpenBSD, so I removed all BSD-related stubs
* Also, removed blur. Don't really need it, maybe will add it later, after
major code refactor.

Features:
* Loading configurations from .ini config files with [this library](https://github.com/rxi/ini)
* Keyboard indicator and Caps Lock layout
* Separated configuration variables

Just copy test_config.ini to $HOME/.config/i3lock-fancier/config.ini and
configure it as you like using provided commentaries.

Dependencies:
* Arch: install `cairo, libev, libx11, pam, xcb-util-image, xcb-util-keysyms, libxkbcommon-x11`
* Debian-based: do `sudo apt install pkg-config libxcb1 libpam-dev libcairo2-dev libxcb-composite0 libxcb-composite0-dev libxcb-xinerama0-dev libev-dev libx11-dev libx11-xcb-dev libxkbcommon0 libxkbcommon-x11-0 libxcb-dpms0-dev libxcb-image0-dev libxcb-util0-dev libxcb-xkb-dev libxkbfile-dev libxkbcommon-x11-dev libxkbcommon-dev`
