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

int ignore_empty_password 		= 1;
int skip_repeated_empty_password 	= 1;

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
char key_x_expr[32]		= "x + (w / 2)\0";
char key_y_expr[32]		= "y + 60 + (h / 2)\0";

double time_size 		= 32.0;
double date_size 		= 14.0;
double text_size 		= 28.0;
double indicators_size 		= 28.0;
double modifier_size 		= 14.0;
double circle_radius 		= 90.0;

char * image_path		= NULL;

char * verif_text		= "Verifying...";
char * wrong_text		= "Wrong Password!";

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
	// const int flag;
	// const float val;

	/** Parse [i3lock] section **/
	ini_sget(config, "i3lock", "debug", "%d", &debug_mode);
	ini_sget(config, "i3lock", "ignore_empty_password", "%d", &ignore_empty_password);
	ini_sget(config, "i3lock", "skip_empty_password", "%d", &skip_repeated_empty_password);
	ini_sget(config, "i3lock", "tile", "%d", &tile);

	ini_sget(config, "i3lock", "screen_number", "%d", &screen_number);
	ini_sget(config, "i3lock", "internal_line_source", "%d", &internal_line_source);

	ini_sget(config, "i3lock", "image_path", NULL, &image_path);

	/** Parse [text] section **/
	ini_sget(config, "text", "verif_text", NULL, &verif_text);
	ini_sget(config, "text", "wrong_text", NULL, &wrong_text);

	ini_sget(config, "text", "text_size", "%f", &text_size);
	ini_sget(config, "text", "modifier_size", "%f", &modifier_size);

	/** Parse [unlock] section **/

	/* Parse [colors] section */
	/* parse background color */
	if ((arg = ini_get(config, "colors", "color")) != NULL) {
		if (arg[0] == '#') arg++; /* Skip # if present */

		if (strlen(arg) != 6
		|| sscanf(arg, "%06[0-9a-fA-F]", color) != 1) {
			errx(EXIT_FAILURE, "color is invalid, "
			"it must be given in 3-byte hexadecimal format: rrggbb\n");
		}
	}

	/* parse all other colors */


	return 0;
}

void free_config(void)
{
	ini_free(config);
}
