#ifndef COLORS_H
#define COLORS_H

#include "settings.h"

/**
 * Color variables and functions
 */

/* Transform hex color code to array of integers */
void color_hex_to_long (char hex[9], uint32_t longs[4]);

/* Set cairo source rgba from array of longs */
void cairo_set_source_rgba_long(cairo_t * tx, uint32_t longs[4]);

/* init long arrays from character arrays */
void make_colors();

extern uint32_t insidever16[4];
extern uint32_t insidewrong16[4];
extern uint32_t inside16[4];

extern uint32_t ringver16[4];
extern uint32_t ringwrong16[4];
extern uint32_t ring16[4];

extern uint32_t line16[4];
extern uint32_t text16[4];
extern uint32_t indicator16[4];
extern uint32_t time16[4];
extern uint32_t date16[4];

extern uint32_t keyhl16[4];
extern uint32_t bshl16[4];
extern uint32_t sep16[4];

#endif // COLORS_H
