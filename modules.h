#ifndef MODULES_H
#define MODULES_H

#include "xcb.h"

#include "settings.h"

#include <cairo.h>
#include <cairo/cairo-xcb.h>
#include <xcb/xcb.h>

extern double circle_radius;

#define BUTTON_RADIUS (circle_radius)
#define BUTTON_SPACE (BUTTON_RADIUS + 5)
#define BUTTON_CENTER (BUTTON_RADIUS + 5)
#define BUTTON_DIAMETER (2 * BUTTON_SPACE)
#define CLOCK_WIDTH 400
#define CLOCK_HEIGHT 200

#define INDICATORS_WIDTH 300
#define INDICATORS_HEIGHT 300

/* A Cairo surface containing the specified image, if any. */
extern cairo_surface_t *img;

/* The root screen, to determine the DPI. */
extern xcb_screen_t *screen;

/**
 * Declarations of the functions to draw on surface and render to context
 * of specific modules, such as clock, keyboard layout, etc.
 */

/*
 * Returns the scaling factor of the current screen.
 * E.g., on a 227 DPI MacBook Pro 13" Retina screen,
 * the scaling factor is 227/96 = 2.36.
 */
double scaling_factor(void);

/* Background color/image */
void draw_background(cairo_t *, uint32_t *);

#endif // MODULES_H
