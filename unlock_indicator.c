#include "colors.h"
#include "modules.h"
#include "unlock_indicator.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern int failed_attempts;
extern char *modifier_string;

void draw_unlock_indicator(cairo_t * ctx, auth_state_t auth_state) {
	cairo_scale(ctx, scaling_factor(), scaling_factor());
	/* Draw a (centered) circle with transparent background. */
	cairo_set_line_width(ctx, 7.0);
	cairo_arc(ctx,
		BUTTON_CENTER /* x */,
		BUTTON_CENTER /* y */,
		BUTTON_RADIUS /* radius */,
		0 /* start */,
		2 * M_PI /* end */
	);

	/* Use the appropriate color for the different PAM states
	 * (currently verifying, wrong password, or default) */
	switch (auth_state) {
	case STATE_AUTH_VERIFY:
	case STATE_AUTH_LOCK:
		cairo_set_source_rgba_long(ctx, insidever16);
		break;
	case STATE_AUTH_WRONG:
	case STATE_I3LOCK_LOCK_FAILED:
		cairo_set_source_rgba_long(ctx, insidewrong16);
		break;
	default:
		cairo_set_source_rgba_long(ctx, inside16);
		break;
	}

	cairo_fill_preserve(ctx);

	switch (auth_state) {
	case STATE_AUTH_VERIFY:
	case STATE_AUTH_LOCK:
		cairo_set_source_rgba_long(ctx, ringver16);
		if (internal_line_source == 1)
			memcpy(line16, ringver16, sizeof(uint32_t) * 4);

		break;

	case STATE_AUTH_WRONG:
	case STATE_I3LOCK_LOCK_FAILED:
		cairo_set_source_rgba_long(ctx, ringwrong16);
		if (internal_line_source == 1)
			memcpy(line16, ringwrong16, sizeof(uint32_t) * 4);

		break;

	case STATE_AUTH_IDLE:
		cairo_set_source_rgba_long(ctx, ring16);
		if (internal_line_source == 1)
			memcpy(line16, ring16, sizeof(uint32_t) * 4);

		break;
	}
	cairo_stroke(ctx);

	/*
	 * Draw an inner separator line.
	 * Pretty sure this only needs drawn if it's being
	 * drawn over the inside?
	 */
	if (internal_line_source != 2) {
		cairo_set_source_rgba_long(ctx, line16);
		cairo_set_line_width(ctx, 2.0);
		cairo_arc(ctx,
			BUTTON_CENTER /* x */,
			BUTTON_CENTER /* y */,
			BUTTON_RADIUS - 5 /* radius */,
			0,
			2 * M_PI);
		cairo_stroke(ctx);
	}

	cairo_set_line_width(ctx, 10.0);

	const char *text = NULL;

	/* We don't want to show more than a 3-digit number. */
	char buf[4];
	cairo_set_source_rgba_long(ctx, text16);
	cairo_select_font_face(ctx,
		"sans-serif",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_NORMAL
	);

	cairo_set_font_size(ctx, text_size);
	switch (auth_state) {
	case STATE_AUTH_VERIFY:
		text = verif_text;
		break;
	case STATE_AUTH_LOCK:
		text = "locking…";
		break;
	case STATE_AUTH_WRONG:
		text = wrong_text;
		break;
	case STATE_I3LOCK_LOCK_FAILED:
		text = "lock failed!";
		break;
	default:
		if (show_failed_attempts && failed_attempts > 0) {
			if (failed_attempts > 999) {
				text = "> 999";
			} else {
				snprintf(buf, sizeof(buf), "%d", failed_attempts);
				text = buf;
			}
			cairo_set_font_size(ctx, 32.0);
		}
		break;
	}

	if (text) {
		cairo_text_extents_t extents;
		double x, y;

		cairo_text_extents(ctx, text, &extents);
		x = BUTTON_CENTER - ((extents.width / 2) + extents.x_bearing);
		y = BUTTON_CENTER - ((extents.height / 2) + extents.y_bearing);

		cairo_move_to(ctx, x, y);
		cairo_show_text(ctx, text);
		cairo_close_path(ctx);
	}

	if (auth_state == STATE_AUTH_WRONG && (modifier_string != NULL)) {
		cairo_text_extents_t extents;
		double x, y;

		cairo_set_font_size(ctx, modifier_size);

		cairo_text_extents(ctx, modifier_string, &extents);
		x = BUTTON_CENTER - ((extents.width / 2) + extents.x_bearing);
		y = BUTTON_CENTER - ((extents.height / 2) + extents.y_bearing) + 28.0;

		cairo_move_to(ctx, x, y);
		cairo_show_text(ctx, modifier_string);
		cairo_close_path(ctx);
	}

}
