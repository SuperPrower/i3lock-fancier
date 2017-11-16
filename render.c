/*
 * © 2010 Michael Stapelberg
 *
 * See LICENSE for licensing information
 *
 */
#include "render.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ev.h>

#include <cairo.h>
#include <cairo/cairo-xcb.h>

#include <time.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#include "i3lock.h"
#include "settings.h"

#include "modules.h"
#include "colors.h"
#include "unlock_indicator.h"
#include "keyboard_layout.h"

#include "xinerama.h"
#include "tinyexpr.h"

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

/* Number of failed unlock attempts. */
extern int failed_attempts;



/*******************************************************************************
 * Local variables.
 ******************************************************************************/

/* time stuff */
static struct ev_periodic *time_redraw_tick;

/* Cache the screen’s visual, necessary for creating a Cairo context. */
static xcb_visualtype_t *vistype;

/* Maintain the current unlock/PAM state to draw the appropriate unlock
 * indicator. */
unlock_state_t unlock_state;
auth_state_t auth_state;


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

	/* draw backgroud */
	draw_background(xcb_ctx, resolution);

	/* build indicator color arrays */
	make_colors();

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
		draw_unlock_indicator(ctx, auth_state);
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

	if (show_keyboard_layout || show_caps_lock_state) {
		draw_keyboard_layout(ind_ctx);
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

	/* XXX: Free them */
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

	te_free(te_unlock_x_expr);
	te_free(te_unlock_y_expr);
	te_free(te_time_x_expr);
	te_free(te_time_y_expr);
	te_free(te_date_x_expr);
	te_free(te_date_y_expr);
	te_free(te_key_x_expr);
	te_free(te_key_y_expr);

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
