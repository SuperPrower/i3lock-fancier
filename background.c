#include "modules.h"

#include <stdlib.h>

void draw_background(cairo_t * xcb_ctx, uint32_t * resolution)
{
	if (img) {
		if (!tile) {
			cairo_set_source_surface(xcb_ctx, img, 0, 0);
			cairo_paint(xcb_ctx);
		} else {
			/* create a pattern and fill a rectangle as big as the screen */
			cairo_pattern_t *pattern;
			pattern = cairo_pattern_create_for_surface(img);
			cairo_set_source(xcb_ctx, pattern);
			cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
			cairo_rectangle(xcb_ctx, 0, 0, resolution[0], resolution[1]);
			cairo_fill(xcb_ctx);
			cairo_pattern_destroy(pattern);
		}

	} else {
		char strgroups[3][3] = {
			{color[0], color[1], '\0'},
			{color[2], color[3], '\0'},
			{color[4], color[5], '\0'}
		};
		uint32_t rgb16[3] = {
			(strtol(strgroups[0], NULL, 16)),
			(strtol(strgroups[1], NULL, 16)),
			(strtol(strgroups[2], NULL, 16))
		};
		cairo_set_source_rgb(
				xcb_ctx,
				rgb16[0] / 255.0,
				rgb16[1] / 255.0,
				rgb16[2] / 255.0
		);
		cairo_rectangle(xcb_ctx, 0, 0, resolution[0], resolution[1]);
		cairo_fill(xcb_ctx);
	}
}
