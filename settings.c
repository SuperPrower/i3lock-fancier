#include "ini.h"

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <cairo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

/*
 * This file stores all setings and options,
 * as well as functions to read them from
 * configuration file.
 *
 * For explanation on flags, check settings.h
 */

/*
 * This structure is wrapper on base configuration variables and their
 * names for easier parsing
 */

/** Colors **/
char color[7] 			= "ffffff";

char insidevercolor[9] 		= "006effbf";
char insidewrongcolor[9] 	= "fa0000bf";
char insidecolor[9] 		= "000000bf";
char ringvercolor[9] 		= "3300faff";
char ringwrongcolor[9] 		= "7d3300ff";
char ringcolor[9] 		= "337d00ff";
char linecolor[9] 		= "000000ff";
char textcolor[9] 		= "000000ff";
char timecolor[9] 		= "000000ff";
char datecolor[9] 		= "000000ff";
char keyhlcolor[9] 		= "33db00ff";
char bshlcolor[9] 		= "db3300ff";
char separatorcolor[9] 		= "000000ff";
char indicatorscolor[9]		= "ffffffff";

int screen_number 		= -1;
int internal_line_source 	= 0;

/** Clock options **/
int show_clock 			= 0;
double refresh_rate 		= 1.0;

/** Flags **/
int unlock_indicator 		= 1;
int always_show_indicator 	= 0;

int show_caps_lock_state 	= 1;
int show_keyboard_layout 	= 1;

int tile 			= 0;

int ignore_empty_password 	= 1;
int show_failed_attempts	= 0;

int beep 			= 0;
int debug_mode 			= 0;

/** Time and Indicator options **/
char time_format[32] 		= "%H:%M:%S\0";
char date_format[32] 		= "%A, %m %Y\0";
char time_font[32] 		= "sans-serif\0";
char date_font[32] 		= "sans-serif\0";
char keyl_font[32]		= "sans-serif\0";
char unlock_x_expr[32] 		= "x + (w / 2)\0";
char unlock_y_expr[32] 		= "y + (h / 2)\0";
char time_x_expr[32] 		= "ix - (cw / 2)\0";
char time_y_expr[32] 		= "iy - (ch / 2)\0";
char date_x_expr[32] 		= "tx\0";
char date_y_expr[32] 		= "ty+30\0";
char key_x_expr[32]		= "x - 500 + (w / 2)\0";
char key_y_expr[32]		= "y - 500 + (h / 2)\0";

double time_size 		= 32.0;
double date_size 		= 14.0;
double text_size 		= 28.0;
double indicators_size 		= 28.0;
double modifier_size 		= 14.0;
double circle_radius 		= 90.0;

char image_path[256]		= {0};

char verif_text[64]		= "Verifying...\0";
char wrong_text[64] 		= "Wrong Password!\0";

static ini_t * config;

/** Configuration file functions prototypes **/
int read_config(char * file)
{
	if (!file || file[0] == '\0') {
		errx(EXIT_FAILURE, "Invalid configuration file path\n");
	}

	/** open config file **/
	config = ini_load(file);
	if (!config) {
		errx(EXIT_FAILURE, "Unable to load configuration file\n");
	}

	/** parse config file **/
	/* temporary variables for checks */
	const char * arg;

	/** Parse [i3lock] section **/
	ini_sget(config, "i3lock", "debug", "%d", &debug_mode);
	ini_sget(config, "i3lock", "show_failed_attempts", "%d", &show_failed_attempts);
	ini_sget(config, "i3lock", "ignore_empty_password", "%d", &ignore_empty_password);
	ini_sget(config, "i3lock", "tile", "%d", &tile);

	ini_sget(config, "i3lock", "screen_number", "%d", &screen_number);
	ini_sget(config, "i3lock", "internal_line_source", "%d", &internal_line_source);


	if ((arg = ini_get(config, "i3lock", "image_path")) != NULL)
		strcpy(image_path, arg);

	/** Parse [text] section **/
	if ((arg = ini_get(config, "text", "verif_text")) != NULL)
		strcpy(verif_text, arg);

	if ((arg = ini_get(config, "text", "wrong_text")) != NULL)
		strcpy(wrong_text, arg);

	ini_sget(config, "text", "text_size", "%f", &text_size);
	ini_sget(config, "text", "modifier_size", "%f", &modifier_size);

	/** Parse [unlock] section **/
	ini_sget(config, "unlock", "show_indicator", "%d", &unlock_indicator);
	ini_sget(config, "unlock", "always_show_indicator", "%d", &always_show_indicator);

	if ((arg = ini_get(config, "unlock", "unlock_x_expr")) != NULL)
		strcpy(unlock_x_expr, arg);

	if ((arg = ini_get(config, "unlock", "unlock_y_expr")) != NULL)
		strcpy(unlock_y_expr, arg);

	ini_sget(config, "unlock", "circle_radius", "%f", &circle_radius);

	/* Parse [colors] section */
	/* parse background color */
	if ((arg = ini_get(config, "colors", "color")) != NULL) {
		if (arg[0] == '#') arg++; /* Skip # if present */

		if (strlen(arg) != 6 || sscanf(arg, "%06[0-9a-fA-F]", color) != 1) {
			errx(EXIT_FAILURE, "color is invalid, "
			"it must be given in 3-byte hexadecimal format: rrggbb\n");
		}
	}

	/* parse all other 4-byte colors */
	arg = ini_get(config, "colors", "insidevercolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", insidevercolor) != 1) {
			errx(EXIT_FAILURE, "insidevercolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "insidewrongcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", insidewrongcolor) != 1) {
			errx(EXIT_FAILURE, "insidewrongcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "insidecolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", insidecolor) != 1) {
			errx(EXIT_FAILURE, "insidecolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}

	arg = ini_get(config, "colors", "ringvercolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", ringvercolor) != 1) {
			errx(EXIT_FAILURE, "ringvercolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "ringwrongcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", ringwrongcolor) != 1) {
			errx(EXIT_FAILURE, "ringwrongcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "ringcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", ringcolor) != 1) {
			errx(EXIT_FAILURE, "ringcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "linecolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", linecolor) != 1) {
			errx(EXIT_FAILURE, "linecolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "textcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", textcolor) != 1) {
			errx(EXIT_FAILURE, "textcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "timecolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", timecolor) != 1) {
			errx(EXIT_FAILURE, "timecolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "datecolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", datecolor) != 1) {
			errx(EXIT_FAILURE, "datecolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}

	arg = ini_get(config, "colors", "keyhlcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", keyhlcolor) != 1) {
			errx(EXIT_FAILURE, "keyhlcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "bshlcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", bshlcolor) != 1) {
			errx(EXIT_FAILURE, "bshlcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "separatorcolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", separatorcolor) != 1) {
			errx(EXIT_FAILURE, "separatorcolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}
	arg = ini_get(config, "colors", "indicatorscolor");
	if (arg) {
		if (arg[0] == '#') arg++;
		if (strlen(arg) != 8 || sscanf(arg, "%08[0-9a-fA-F]", indicatorscolor) != 1) {
			errx(EXIT_FAILURE, "indicatorscolor is invalid, "
			"it must be given in 4-byte hexadecimal format: rrggbbaa\n");
		}
	}

	/* parse [clock] section */
	ini_sget(config, "clock", "show_clock", "%d", &show_clock);
	ini_sget(config, "clock", "refresh_rate", "%f", &refresh_rate);

	if ((arg = ini_get(config, "clock", "format")) != NULL)
		strcpy(time_format, arg);

	if ((arg = ini_get(config, "clock", "font")) != NULL)
		strcpy(time_font, arg);

	if ((arg = ini_get(config, "clock", "x_expr")) != NULL)
		strcpy(time_x_expr, arg);

	if ((arg = ini_get(config, "clock", "y_expr")) != NULL)
		strcpy(time_y_expr, arg);

	ini_sget(config, "clock", "font_size", "%f", &time_size);

	/* parse [date] section */
	if ((arg = ini_get(config, "date", "format")) != NULL)
		strcpy(date_format, arg);

	if ((arg = ini_get(config, "date", "font")) != NULL)
		strcpy(date_font, arg);

	ini_sget(config, "date", "font_size", "%f", &date_size);

	if ((arg = ini_get(config, "date", "x_expr")) != NULL)
		strcpy(date_x_expr, arg);

	if ((arg = ini_get(config, "date", "y_expr")) != NULL)
		strcpy(date_y_expr, arg);

	/* parse [keyboard] section */
	ini_sget(config, "keyboard", "show_key_layout", "%d", &show_keyboard_layout);
	ini_sget(config, "keyboard", "show_caps_state", "%d", &show_caps_lock_state);

	if ((arg = ini_get(config, "keyboard", "font")) != NULL)
		strcpy(keyl_font, arg);

	ini_sget(config, "keyboard", "font_size", "%f", &indicators_size);

	if ((arg = ini_get(config, "keyboard", "x_expr")) != NULL)
		strcpy(key_x_expr, arg);

	if ((arg = ini_get(config, "keyboard", "y_expr")) != NULL)
		strcpy(key_y_expr, arg);

	ini_free(config);

	return 0;
}
