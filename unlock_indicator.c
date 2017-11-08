/*
 * © 2010 Michael Stapelberg
 *
 * See LICENSE for licensing information
 *
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <xcb/xcb.h>
#include <ev.h>
#include <cairo.h>
#include <cairo/cairo-xcb.h>

#include "settings.h"
#include "i3lock.h"
#include "xcb.h"
#include "unlock_indicator.h"
#include "xinerama.h"
#include "tinyexpr.h"

/* clock stuff */
#include <time.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

extern double circle_radius;

#define BUTTON_RADIUS (circle_radius)
#define BUTTON_SPACE (BUTTON_RADIUS + 5)
#define BUTTON_CENTER (BUTTON_RADIUS + 5)
#define BUTTON_DIAMETER (2 * BUTTON_SPACE)
#define CLOCK_WIDTH 400
#define CLOCK_HEIGHT 200

#define INDICATORS_WIDTH 300
#define INDICATORS_HEIGHT 300

/*******************************************************************************
 * Variables defined in i3lock.c and settings.c
 ******************************************************************************/

/* The current position in the input buffer. Useful to determine if any
 * characters of the password have already been entered or not. */
int input_position;

/* The lock window. */
extern xcb_window_t win;

/* The current resolution of the X11 root window. */
extern uint32_t last_resolution[2];

/* List of pressed modifiers, or NULL if none are pressed. */
extern char *modifier_string;

/* A Cairo surface containing the specified image (-i), if any. */
extern cairo_surface_t *img;
extern cairo_surface_t *blur_img;

/* Number of failed unlock attempts. */
extern int failed_attempts;

/*
 * The display, to determine if Caps Lock is triggered
 * and to determine keyboard layout
 */
extern Display* _display;

/* Strings representing keyboard layouts group */
extern char kb_layouts_group[][3];

/*******************************************************************************
 * Variables defined in xcb.c.
 ******************************************************************************/

/* The root screen, to determine the DPI. */
extern xcb_screen_t *screen;

/*******************************************************************************
 * Local variables.
 ******************************************************************************/

/* time stuff */
static struct ev_periodic *time_redraw_tick;

/* Caps lock state string showing when caps lock is active */
char CAPS_LOCK_STRING[] = "CAPS";

/* Cache the screen’s visual, necessary for creating a Cairo context. */
static xcb_visualtype_t *vistype;

/* Maintain the current unlock/PAM state to draw the appropriate unlock
 * indicator. */
unlock_state_t unlock_state;
auth_state_t auth_state;

/* Keyboard state to determine keyboard layout */
extern XkbStateRec xkbState;

/* Used to determine caps lock state */
XKeyboardState xKeyboardState;

/*
 * Returns the scaling factor of the current screen.
 * E.g., on a 227 DPI MacBook Pro 13" Retina screen,
 * the scaling factor is 227/96 = 2.36.
 */
static double scaling_factor(void)
{
	const int dpi = (double)screen->height_in_pixels * 25.4 /
		(double)screen->height_in_millimeters;
	return (dpi / 96.0);
}

/** Helper functions to reduce number of code in function below **/
/* Transform hex color code to array of integers */
void color_hex_to_long (char hex[9], uint32_t longs[4])
{
	char split[4][3] = {
		{hex[0], hex[1], '\0'},
		{hex[2], hex[3], '\0'},
		{hex[4], hex[5], '\0'},
		{hex[6], hex[7], '\0'}
	};

	longs[0] = (strtol(split[0], NULL, 16));
	longs[1] = (strtol(split[1], NULL, 16));
	longs[2] = (strtol(split[2], NULL, 16));
	longs[3] = (strtol(split[3], NULL, 16));
}

/* Set cairo source rgba from array of longs */
void cairo_set_source_rgba_long(cairo_t * tx, uint32_t longs[4])
{
	cairo_set_source_rgba(tx,
		(double)longs[0]/255,
		(double)longs[1]/255,
		(double)longs[2]/255,
		(double)longs[3]/255
	);
}

/*
 * Draws global image with fill color onto a pixmap with the given
 * resolution and returns it.
 */
xcb_pixmap_t draw_image(uint32_t *resolution) {
	xcb_pixmap_t bg_pixmap = XCB_NONE;
	int button_diameter_physical = ceil(scaling_factor() * BUTTON_DIAMETER);
	int clock_width_physical = ceil(scaling_factor() * CLOCK_WIDTH);
	int clock_height_physical = ceil(scaling_factor() * CLOCK_HEIGHT);
	int indicators_height_physical = ceil(scaling_factor() * INDICATORS_HEIGHT);
	int indicators_width_physical = ceil(scaling_factor() * INDICATORS_WIDTH);
	DEBUG("scaling_factor is %.f, physical diameter is %d px\n",
			scaling_factor(), button_diameter_physical);

	if (!vistype) vistype = get_root_visual_type(screen);
	bg_pixmap = create_bg_pixmap(conn, screen, resolution, color);
	/*
	 * Initialize cairo: Create one in-memory surface to render the unlock
	 * indicator on, create one XCB surface to actually draw (one or more,
	 * depending on the amount of screens) unlock indicators on.
	 * Also, create two more surfaces for time and date display
	 * and one more for keyboard layout and caps lock indicator
	 */
	cairo_surface_t *output = cairo_image_surface_create(
			CAIRO_FORMAT_ARGB32,
			button_diameter_physical,
			button_diameter_physical
	);
	cairo_t *ctx = cairo_create(output);

	cairo_surface_t *time_output = cairo_image_surface_create(
			CAIRO_FORMAT_ARGB32,
			clock_width_physical,
			clock_height_physical
	);
	cairo_t *time_ctx = cairo_create(time_output);

	cairo_surface_t *date_output = cairo_image_surface_create(
			CAIRO_FORMAT_ARGB32,
			clock_width_physical,
			clock_height_physical
	);
	cairo_t *date_ctx = cairo_create(date_output);

	cairo_surface_t *indicators_output = cairo_image_surface_create(
			CAIRO_FORMAT_ARGB32,
			indicators_width_physical,
			indicators_height_physical
	);
	cairo_t *ind_ctx = cairo_create(indicators_output);

	cairo_surface_t *xcb_output = cairo_xcb_surface_create(
			conn, bg_pixmap,
			vistype,
			resolution[0],
			resolution[1]
	);
	cairo_t *xcb_ctx = cairo_create(xcb_output);

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

	/* build indicator color arrays */
	uint32_t insidever16[4];
	color_hex_to_long(insidevercolor, insidever16);

	uint32_t insidewrong16[4];
	color_hex_to_long(insidewrongcolor, insidewrong16);

	uint32_t inside16[4];
	color_hex_to_long(insidecolor, inside16);

	uint32_t ringver16[4];
	color_hex_to_long(ringvercolor, ringver16);

	uint32_t ringwrong16[4];
	color_hex_to_long(ringwrongcolor, ringwrong16);

	uint32_t ring16[4];
	color_hex_to_long(ringcolor, ring16);

	uint32_t line16[4];
	color_hex_to_long(linecolor, line16);

	uint32_t text16[4];
	color_hex_to_long(textcolor, text16);

	uint32_t indicator16[4];
	color_hex_to_long(indicatorscolor, indicator16);

	uint32_t time16[4];
	color_hex_to_long(timecolor, time16);

	uint32_t date16[4];
	color_hex_to_long(datecolor, date16);

	uint32_t keyhl16[4];
	color_hex_to_long(keyhlcolor, keyhl16);

	uint32_t bshl16[4];
	color_hex_to_long(bshlcolor, bshl16);

	uint32_t sep16[4];
	color_hex_to_long(separatorcolor, sep16);

	/* https://github.com/ravinrabbid/i3lock-clock/commit/0de3a411fa5249c3a4822612c2d6c476389a1297 */
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	if (unlock_indicator &&
		(unlock_state >= STATE_KEY_PRESSED
		|| auth_state > STATE_AUTH_IDLE
		|| always_show_indicator)
	) {
		cairo_scale(ctx, scaling_factor(), scaling_factor());
		/* Draw a (centered) circle with transparent background. */
		cairo_set_line_width(ctx, 7.0);
		cairo_arc(ctx,
				BUTTON_CENTER /* x */,
				BUTTON_CENTER /* y */,
				BUTTON_RADIUS /* radius */,
				0 /* start */,
				2 * M_PI /* end */);

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

	/** Draw Keyboard Layout and Caps Lock Indicator **/
	int ind_x = INDICATORS_WIDTH / 2;
	int ind_y = INDICATORS_HEIGHT / 2;

	if (show_keyboard_layout || show_caps_lock_state) {
		cairo_set_source_rgba_long(ind_ctx, indicator16);

		cairo_select_font_face(ind_ctx,
			keyl_font,
			CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL
		);

		cairo_set_font_size(ind_ctx, indicators_size);


		/* Get Keyboard Layout boundaries */
		XkbGetState(_display, XkbUseCoreKbd, &xkbState);
		char *kb_layout = kb_layouts_group[xkbState.group];
		cairo_text_extents_t kb_layout_extents;
		cairo_text_extents(ind_ctx, kb_layout, &kb_layout_extents);

		/* Get Caps Lock indicator boundaries */
		cairo_text_extents_t caps_extents;
		cairo_text_extents(ind_ctx, CAPS_LOCK_STRING, &caps_extents);

		// Keyboard layout
		if (show_keyboard_layout) {


			double kb_layout_x
				= ind_x
				- kb_layout_extents.width / 2
				- kb_layout_extents.x_bearing;
			double kb_layout_y
				= ind_y
				- kb_layout_extents.height / 2
				- kb_layout_extents.y_bearing;

			cairo_move_to(ind_ctx, kb_layout_x, kb_layout_y);
			cairo_show_text(ind_ctx, kb_layouts_group[xkbState.group]);
			cairo_close_path(ind_ctx);
		}

		/* Caps Lock state */
		if (show_caps_lock_state) {
			double caps_lock_state_x
				= ind_x
				- caps_extents.width / 2
				- caps_extents.x_bearing;

			double caps_lock_state_y
				= ind_y
				- caps_extents.height / 2
				- caps_extents.y_bearing
				+ 1.5 * caps_extents.height;

			XGetKeyboardControl(_display, &xKeyboardState);
			/* if caps lock is switched on */
			if (xKeyboardState.led_mask % 2 == 1) {
				cairo_move_to(ind_ctx, caps_lock_state_x, caps_lock_state_y);
				cairo_show_text(ind_ctx, CAPS_LOCK_STRING);
				cairo_close_path(ind_ctx);
			}
		}

	}


	/*
	 * After the user pressed any valid key or the backspace key, we
	 * highlight a random part of the unlock indicator to confirm this
	 * keypress.
	 */
	if (unlock_state == STATE_KEY_ACTIVE
	||  unlock_state == STATE_BACKSPACE_ACTIVE) {
		cairo_set_line_width(ctx, 7.0);
		cairo_new_sub_path(ctx);
		double highlight_start = (rand() % (int)(2 * M_PI * 100)) / 100.0;
		cairo_arc(ctx,
			BUTTON_CENTER /* x */,
			BUTTON_CENTER /* y */,
			BUTTON_RADIUS /* radius */,
			highlight_start,
			highlight_start + (M_PI / 3.0)
		);
		if (unlock_state == STATE_KEY_ACTIVE) {
			cairo_set_source_rgba_long(ctx, keyhl16);
		} else {
			cairo_set_source_rgba_long(ctx, bshl16);
		}

		cairo_stroke(ctx);

		/* Draw two little separators for the highlighted part of the
		 * unlock indicator. */
		cairo_set_source_rgba_long(ctx, sep16);
		cairo_arc(ctx,
			BUTTON_CENTER /* x */,
			BUTTON_CENTER /* y */,
			BUTTON_RADIUS /* radius */,
			highlight_start /* start */,
			highlight_start + (M_PI / 128.0) /* end */
		);
		cairo_stroke(ctx);
		cairo_arc(ctx,
			BUTTON_CENTER /* x */,
			BUTTON_CENTER /* y */,
			BUTTON_RADIUS /* radius */,
			(highlight_start + (M_PI / 3.0)) - (M_PI / 128.0) /* start */,
			highlight_start + (M_PI / 3.0) /* end */);
		cairo_stroke(ctx);
	}

	if (show_clock) {
		char *text = NULL;
		char *date = NULL;
		char time_text[40] = {0};
		char date_text[40] = {0};

		strftime(time_text, 40, time_format, timeinfo);
		strftime(date_text, 40, date_format, timeinfo);
		text = time_text;
		date = date_text;

		if (text) {
			double x, y;
			cairo_text_extents_t extents;

			cairo_set_font_size(time_ctx, time_size);
			cairo_select_font_face(
				time_ctx, time_font,
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL
			);
			cairo_set_source_rgba_long(time_ctx, time16);

			cairo_text_extents(time_ctx, text, &extents);

			x = (CLOCK_WIDTH / 2)
				- ((extents.width / 2) + extents.x_bearing);
			y = CLOCK_HEIGHT / 2;

			cairo_move_to(time_ctx, x, y);
			cairo_show_text(time_ctx, text);
			cairo_close_path(time_ctx);
		}

		if (date) {
			double x, y;
			cairo_text_extents_t extents;

			cairo_select_font_face(date_ctx,
				date_font,
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL
			);
			cairo_set_source_rgba_long(date_ctx, date16);
			cairo_set_font_size(date_ctx, date_size);

			cairo_text_extents(date_ctx, date, &extents);
			x = CLOCK_WIDTH / 2
				- ((extents.width / 2) + extents.x_bearing);
			y = CLOCK_HEIGHT / 2;

			cairo_move_to(date_ctx, x, y);
			cairo_show_text(date_ctx, date);
			cairo_close_path(date_ctx);
		}
	}

	/*
	 * I'm not even going to try to refactor this code.
	 * Screw it. It works. It's not my problem.
	 */
	double unx, uny;
	double x, y;
	double screen_x, screen_y;
	double w, h;
	double tx = 0;
	double ty = 0;
	double indx, indy;

	double clock_width = CLOCK_WIDTH;
	double clock_height = CLOCK_HEIGHT;

	double radius = BUTTON_RADIUS;
	int te_x_err;
	int te_y_err;
	// variable mapping for evaluating the clock position expression
	te_variable vars[] = {
		{"w", &w}, {"h", &h},
		{"x", &screen_x}, {"y", &screen_y},
		{"ix", &unx}, {"iy", &uny},
		{"tx", &tx}, {"ty", &ty},
		{"cw", &clock_width}, {"ch", &clock_height},
		{"r", &radius}
	};

	te_expr *te_unlock_x_expr = te_compile(unlock_x_expr, vars, 11, &te_x_err);
	te_expr *te_unlock_y_expr = te_compile(unlock_y_expr, vars, 11, &te_y_err);
	te_expr *te_time_x_expr = te_compile(time_x_expr, vars, 11, &te_x_err);
	te_expr *te_time_y_expr = te_compile(time_y_expr, vars, 11, &te_y_err);
	te_expr *te_date_x_expr = te_compile(date_x_expr, vars, 11, &te_x_err);
	te_expr *te_date_y_expr = te_compile(date_y_expr, vars, 11, &te_y_err);
	te_expr *te_key_x_expr	= te_compile(key_x_expr, vars, 11, &te_x_err);
	te_expr *te_key_y_expr 	= te_compile(key_y_expr, vars, 11, &te_y_err);

	if (xr_screens > 0) {
		/* Composite the unlock indicator in the middle of each screen. */
		// excuse me, just gonna hack something in right here
		if (screen_number != -1 && screen_number < xr_screens) {
			w = xr_resolutions[screen_number].width;
			h = xr_resolutions[screen_number].height;
			screen_x = xr_resolutions[screen_number].x;
			screen_y = xr_resolutions[screen_number].y;
			if (te_unlock_x_expr && te_unlock_y_expr) {
				unx = 0;
				uny = 0;
				unx = te_eval(te_unlock_x_expr);
				uny = te_eval(te_unlock_y_expr);
				DEBUG("\tscreen x: %d "
					"screen y: %d "
					"screen w: %f "
					"screen h: %f "
					"ix: %f iy: %f\n",
					xr_resolutions[screen_number].x,
					xr_resolutions[screen_number].y,
					w, h,
					unx, uny
				);
			}
			else {
				unx = xr_resolutions[screen_number].x
					+ (xr_resolutions[screen_number].width / 2);

				uny = xr_resolutions[screen_number].y
					+ (xr_resolutions[screen_number].height / 2);
			}

			x = unx - (button_diameter_physical / 2);
			y = uny - (button_diameter_physical / 2);

			cairo_set_source_surface(xcb_ctx, output, x, y);
			cairo_rectangle(xcb_ctx, x, y, button_diameter_physical, button_diameter_physical);
			cairo_fill(xcb_ctx);

			indx = te_eval(te_key_x_expr);
			indy = te_eval(te_key_y_expr);
			cairo_set_source_surface(xcb_ctx, indicators_output, indx - 150, indy - 150);
			cairo_rectangle(xcb_ctx,
					indx - 150,
					indy - 150,
					indicators_width_physical,
					indicators_height_physical
			);
			cairo_fill(xcb_ctx);

			if (te_time_x_expr && te_time_y_expr) {
				tx = 0;
				ty = 0;
				tx = te_eval(te_time_x_expr);
				ty = te_eval(te_time_y_expr);
				double time_x = tx;
				double time_y = ty;
				double date_x = te_eval(te_date_x_expr);
				double date_y = te_eval(te_date_y_expr);
				DEBUG("\ttx: %f "
					"ty: %f "
					"unx: %f, uny: %f\n",
					tx, ty,
					unx, uny
				);
				DEBUG("\ttime_x: %f "
					"time_y: %f "
					"date_x: %f "
					"date_y: %f "
					"screen_number: %d\n",
					time_x, time_y,
					date_x, date_y,
					screen_number
				);
				DEBUG("\tscreen x: %d "
					"screen y: %d "
					"screen w: %f "
					"screen h: %f\n",
					xr_resolutions[screen_number].x,
					xr_resolutions[screen_number].y,
					w, h
				);
				cairo_set_source_surface(xcb_ctx, time_output, time_x, time_y);
				cairo_rectangle(xcb_ctx, time_x, time_y, CLOCK_WIDTH, CLOCK_HEIGHT);
				cairo_fill(xcb_ctx);
				cairo_set_source_surface(xcb_ctx, date_output, date_x, date_y);
				cairo_rectangle(xcb_ctx, date_x, date_y, CLOCK_WIDTH, CLOCK_HEIGHT);
				cairo_fill(xcb_ctx);
			}

		} else {
			for (int screen = 0; screen < xr_screens; screen++) {
				w = xr_resolutions[screen].width;
				h = xr_resolutions[screen].height;
				screen_x = xr_resolutions[screen].x;
				screen_y = xr_resolutions[screen].y;
				if (te_unlock_x_expr && te_unlock_y_expr) {
					unx = 0;
					uny = 0;
					unx = te_eval(te_unlock_x_expr);
					uny = te_eval(te_unlock_y_expr);
					DEBUG("\tscreen x: %d "
						"screen y: %d "
						"screen w: %f "
						"screen h: %f "
						"unx: %f uny: %f\n",
						xr_resolutions[screen].x,
						xr_resolutions[screen].y,
						w, h,
						unx, uny
					);
				} else {
					unx = xr_resolutions[screen].x + (xr_resolutions[screen].width / 2);
					uny = xr_resolutions[screen].y + (xr_resolutions[screen].height / 2);
				}
				x = unx - (button_diameter_physical / 2);
				y = uny - (button_diameter_physical / 2);
				cairo_set_source_surface(xcb_ctx, output, x, y);
				cairo_rectangle(xcb_ctx,
						x, y,
						button_diameter_physical,
						button_diameter_physical
				);
				cairo_fill(xcb_ctx);

				/** Draw Keyboard indicator **/
				/** Draw currently happens here **/
				indx = te_eval(te_key_x_expr);
				indy = te_eval(te_key_y_expr);

				cairo_set_source_surface(xcb_ctx,
						indicators_output,
						indx - 150,
						indy - 150
				);
				cairo_rectangle(xcb_ctx,
						indx - 150,
						indy - 150,
						indicators_width_physical,
						indicators_height_physical
				);
				cairo_fill(xcb_ctx);


				if (te_time_x_expr && te_time_y_expr) {
					tx = 0;
					ty = 0;
					tx = te_eval(te_time_x_expr);
					ty = te_eval(te_time_y_expr);
					double time_x = tx;
					double time_y = ty;
					double date_x = te_eval(te_date_x_expr);
					double date_y = te_eval(te_date_y_expr);
					DEBUG("\ttx: %f "
						"ty: %f "
						"unx: %f "
						"uny: %f\n",
						tx, ty,
						unx, uny
					);
					DEBUG("\ttime_x: %f "
						"time_y: %f "
						"date_x: %f "
						"date_y: %f "
						"screen_number: %d\n",
						time_x, time_y,
						date_x, date_y,
						screen
					);
					DEBUG("\tscreen x: %d "
						"screen y: %d "
						"screen w: %f "
						"screen h: %f\n",
						xr_resolutions[screen].x,
						xr_resolutions[screen].y,
						w, h
					);
					cairo_set_source_surface(xcb_ctx,
							time_output,
							time_x, time_y
					);
					cairo_rectangle(xcb_ctx,
							time_x, time_y,
							CLOCK_WIDTH,
							CLOCK_HEIGHT
					);
					cairo_fill(xcb_ctx);
					cairo_set_source_surface(xcb_ctx,
							date_output,
							date_x, date_y
					);
					cairo_rectangle(xcb_ctx,
							date_x, date_y,
							CLOCK_WIDTH,
							CLOCK_HEIGHT
					);
					cairo_fill(xcb_ctx);
				} else {
					DEBUG("\terror codes for exprs are "
						"%d, %d\n",
						te_x_err, te_y_err
					);
					DEBUG("\texprs: %s, %s\n",
						time_x_expr, time_y_expr
					);
				}
			}
		}
	} else {
		/* We have no information about the screen sizes/positions, so we just
		 * place the unlock indicator in the middle of the X root window and
		 * hope for the best. */
		w = last_resolution[0];
		h = last_resolution[1];
		unx = last_resolution[0] / 2;
		uny = last_resolution[1] / 2;
		x = unx - (button_diameter_physical / 2);
		y = uny - (button_diameter_physical / 2);
		cairo_set_source_surface(xcb_ctx, output, x, y);
		cairo_rectangle(xcb_ctx, x, y, button_diameter_physical, button_diameter_physical);
		cairo_fill(xcb_ctx);
		if (te_time_x_expr && te_time_y_expr) {
			tx = te_eval(te_time_x_expr);
			ty = te_eval(te_time_y_expr);
			double time_x = tx - CLOCK_WIDTH / 2;
			double time_y = tx - CLOCK_HEIGHT / 2;
			double date_x = te_eval(te_date_x_expr) - CLOCK_WIDTH / 2;
			double date_y = te_eval(te_date_y_expr) - CLOCK_HEIGHT / 2;
			DEBUG("Placing time at %f, %f\n", time_x, time_y);
			cairo_set_source_surface(xcb_ctx, time_output, time_x, time_y);
			cairo_rectangle(xcb_ctx, time_x, time_y, CLOCK_WIDTH, CLOCK_HEIGHT);
			cairo_fill(xcb_ctx);
			cairo_set_source_surface(xcb_ctx, date_output, date_x, date_y);
			cairo_rectangle(xcb_ctx, date_x, date_y, CLOCK_WIDTH, CLOCK_HEIGHT);
			cairo_fill(xcb_ctx);
		}

		/* Draw Keyboard indicator */
		indx = te_eval(te_key_x_expr);
		indy = te_eval(te_key_y_expr);
		cairo_set_source_surface(xcb_ctx,
				indicators_output,
				indx - 150, indy - 150
		);
		cairo_rectangle(xcb_ctx,
				indx - 150, indy - 150,
				indicators_width_physical,
				indicators_height_physical
		);
		cairo_fill(xcb_ctx);
	}

	cairo_surface_destroy(xcb_output);
	cairo_surface_destroy(time_output);
	cairo_surface_destroy(date_output);
	cairo_surface_destroy(indicators_output);
	cairo_surface_destroy(output);
	cairo_destroy(ctx);
	cairo_destroy(time_ctx);
	cairo_destroy(date_ctx);
	cairo_destroy(ind_ctx);
	cairo_destroy(xcb_ctx);
	/* XXX: Free them, and positions equations? */
	return bg_pixmap;
}

/*
 * Calls draw_image on a new pixmap and swaps that with the current pixmap
 *
 */
void redraw_screen(void) {
	DEBUG("redraw_screen(unlock_state = %d, auth_state = %d)\n", unlock_state, auth_state);
	xcb_pixmap_t bg_pixmap = draw_image(last_resolution);
	xcb_change_window_attributes(conn, win, XCB_CW_BACK_PIXMAP, (uint32_t[1]){bg_pixmap});
	/* XXX: Possible optimization: Only update the area in the middle of the
	 * screen instead of the whole screen. */
	xcb_clear_area(conn, 0, win, 0, 0, last_resolution[0], last_resolution[1]);
	xcb_free_pixmap(conn, bg_pixmap);
	xcb_flush(conn);
}

/*
 * Hides the unlock indicator completely when there is no content in the
 * password buffer.
 *
 */
void clear_indicator(void) {
	if (input_position == 0) {
		unlock_state = STATE_STARTED;
	} else
		unlock_state = STATE_KEY_PRESSED;
	redraw_screen();
}

static void time_redraw_cb(struct ev_loop *loop, ev_periodic *w, int revents) {
	redraw_screen();
}

void start_time_redraw_tick(struct ev_loop* main_loop) {
	if (time_redraw_tick) {
		ev_periodic_set(time_redraw_tick, 0., refresh_rate, 0);
		ev_periodic_again(main_loop, time_redraw_tick);
	} else {
		if (!(time_redraw_tick = calloc(sizeof(struct ev_periodic), 1))) {
			return;
		}
		ev_periodic_init(time_redraw_tick, time_redraw_cb, 0., refresh_rate, 0);
		ev_periodic_start(main_loop, time_redraw_tick);
	}
}
