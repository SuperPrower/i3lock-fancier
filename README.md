i3lock-fancier - yet another i3lock fork
========================================

This fork combines features of [i3lock-color](https://github.com/chrjguill/i3lock-color)
and [this i3lock fork](https://github.com/cac03/i3lock/commits/master) to
create my ultimate lockscreen experience.


Features:
* Separated configuration variables and more cleaner code in future
* Loading configurations from .ini config files with [this library](https://github.com/rxi/ini)
* Keyboard indicator and Caps Lock layout:

![Feature showcase](https://raw.githubusercontent.com/SuperPrower/i3lock-fancier/master/feature.png)

Just copy test_config.ini to $HOME/.config/i3lock-fancier/config.ini and
configure it as you like using provided commentaries.

i3lock-fancier is almost completely compatible with i3lock-color, and you can
port your fany config and have cool screenshots like this:

![Cool Screenshot](https://raw.githubusercontent.com/SuperPrower/i3lock-fancier/master/screenshot.png)

Keep in mind:
* I don't know how to work with OpenBSD, so I removed all BSD-related code
* Also, removed blur. Don't really need it, maybe will add it later, after
major code refactor.

Dependencies are mostly inherited from i3lock-color:
* Arch: install `cairo, libev, libx11, pam, xcb-util-image, xcb-util-keysyms, libxkbcommon-x11`
* Debian-based: do `sudo apt install pkg-config libxcb1 libpam-dev libcairo2-dev libxcb-composite0 libxcb-composite0-dev libxcb-xinerama0-dev libev-dev libx11-dev libx11-xcb-dev libxkbcommon0 libxkbcommon-x11-0 libxcb-dpms0-dev libxcb-image0-dev libxcb-util0-dev libxcb-xkb-dev libxkbfile-dev libxkbcommon-x11-dev libxkbcommon-dev`
