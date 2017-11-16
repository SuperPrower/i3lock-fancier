#ifndef _KEYBOARD_LAYOUT_H
#define _KEYBOARD_LAYOUT_H

#include "modules.h"
#include "settings.h"
#include "colors.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#include <cairo.h>

void draw_keyboard_layout(cairo_t * ctx);

#endif
