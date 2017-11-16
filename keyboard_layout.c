#include "keyboard_layout.h"

/*
 * The display, to determine if Caps Lock is triggered
 * and to determine keyboard layout
 */
extern Display* _display;

/* Strings representing keyboard layouts group */
extern char kb_layouts_group[][3];

/* Keyboard state to determine keyboard layout */
extern XkbStateRec xkbState;

/* Used to determine caps lock state */
static XKeyboardState xKeyboardState;

/* Caps lock state string showing when caps lock is active */
static char CAPS_LOCK_STRING[] = "CAPS";

void draw_keyboard_layout(cairo_t * ctx) {
	/** Draw Keyboard Layout and Caps Lock Indicator **/
	int ind_x = INDICATORS_WIDTH / 2;
	int ind_y = INDICATORS_HEIGHT / 2;

	cairo_set_source_rgba_long(ctx, indicator16);

	cairo_select_font_face(ctx,
		keyl_font,
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_NORMAL
	);

	cairo_set_font_size(ctx, indicators_size);


	/* Get Keyboard Layout boundaries */
	XkbGetState(_display, XkbUseCoreKbd, &xkbState);
	char *kb_layout = kb_layouts_group[xkbState.group];
	cairo_text_extents_t kb_layout_extents;
	cairo_text_extents(ctx, kb_layout, &kb_layout_extents);

	/* Get Caps Lock indicator boundaries */
	cairo_text_extents_t caps_extents;
	cairo_text_extents(ctx, CAPS_LOCK_STRING, &caps_extents);

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

		cairo_move_to(ctx, kb_layout_x, kb_layout_y);
		cairo_show_text(ctx, kb_layouts_group[xkbState.group]);
		cairo_close_path(ctx);
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
			cairo_move_to(ctx, caps_lock_state_x, caps_lock_state_y);
			cairo_show_text(ctx, CAPS_LOCK_STRING);
			cairo_close_path(ctx);
		}
	}
}
