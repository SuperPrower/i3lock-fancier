#ifndef I3_LOCK_FANCIER_CONFIG_H
#define I3_LOCK_FANCIER_CONFIG_H

#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <cairo.h>


/** Colors **/
/* The background color to use (in hex). */
extern char color[7];

/* indicator color options */
extern char insidevercolor[9];
extern char insidewrongcolor[9];
extern char insidecolor[9];
extern char ringvercolor[9];
extern char ringwrongcolor[9];
extern char ringcolor[9];
extern char linecolor[9];
extern char textcolor[9];
extern char timecolor[9];
extern char datecolor[9];
extern char keyhlcolor[9];
extern char bshlcolor[9];
extern char separatorcolor[9];
extern char indicatorscolor[9];

/*
 * int defining which display the lock indicator should be shown on.
 * If -1, then show on all displays.
 */
extern int screen_number;
/*
 * default is to use the supplied line color,
 * 1 will be ring color,
 * 2 will be to use the inside color for ver/wrong/etc
 */
extern int internal_line_source;

/** Clock options **/
extern int show_clock;

extern float refresh_rate;

/** Flags **/
/* Whether the unlock indicator is enabled (defaults to true). */
extern int unlock_indicator;
extern int always_show_indicator;
/* Show number of failed attempts, if any */
extern int show_failed_attempts;

/* Allows user to select whether he wants see caps lock state */
extern int show_caps_lock_state;
/* Allows user to select whether he wants see current keyboard layout */
extern int show_keyboard_layout;

/* Whether the image should be tiled. */
extern int tile;

extern int ignore_empty_password;

extern int beep;
extern int debug_mode;

/** Time/Date formatter strings **/
extern char time_format[32];
extern char date_format[32];
/** Time/Date/Layout font string **/
extern char time_font[32];
extern char date_font[32];
extern char keyl_font[32];
/* Unlock indicator position */
extern char unlock_x_expr[32];
extern char unlock_y_expr[32];
/* Clock position */
extern char time_x_expr[32];
extern char time_y_expr[32];
/* Date position */
extern char date_x_expr[32];
extern char date_y_expr[32];
/* Keyboard layout position */
extern char key_x_expr[32];
extern char key_y_expr[32];

extern double time_size;
extern double date_size;
extern double text_size;
extern double indicators_size;
extern double modifier_size;
extern double circle_radius;

extern const char image_path[256];

extern const char verif_text[64];
extern const char wrong_text[64];

/** Configuration file functions prototypes **/
int read_config(char *);

/** Release configuration file (because releasing it invalidates all strings **/
void free_config(void);

#endif // I3_LOCK_FANCIER_CONFIG_H
