i3lock-fancier - yet another i3lock fork
========================================

This fork combines features of [i3lock-color](https://github.com/chrjguill/i3lock-color) and [this i3lock fork](https://github.com/cac03/i3lock/commits/master) to create my ultimate lockscreen experience.

Drawbacks:
* I don't know how to work with OpenBSD, so I removed all BSD-related stubs
* Also, removed blur. Don't really need it, maybe will add it later, after
major code refactor.

Features:
* Loading configurations from .ini config files with [this library](https://github.com/rxi/ini)
* Keyboard indicator and Caps Lock layout
* Separated configuration variables
* Code separation in future
