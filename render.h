#ifndef _RENDER_H
#define _RENDER_H

#include <ev.h>
#include <xcb/xcb.h>

xcb_pixmap_t draw_image(uint32_t* resolution);
void redraw_screen(void);
void clear_indicator(void);
void start_time_redraw_timeout(void);
void start_time_redraw_tick(struct ev_loop*);

#endif
