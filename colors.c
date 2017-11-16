#include "colors.h"

#include <stdlib.h>

uint32_t insidever16[4];
uint32_t insidewrong16[4];
uint32_t inside16[4];

uint32_t ringver16[4];
uint32_t ringwrong16[4];
uint32_t ring16[4];

uint32_t line16[4];
uint32_t text16[4];
uint32_t indicator16[4];
uint32_t time16[4];
uint32_t date16[4];

uint32_t keyhl16[4];
uint32_t bshl16[4];
uint32_t sep16[4];

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

void cairo_set_source_rgba_long(cairo_t * tx, uint32_t longs[4])
{
	cairo_set_source_rgba(tx,
		(double)longs[0]/255,
		(double)longs[1]/255,
		(double)longs[2]/255,
		(double)longs[3]/255
	);
}

void make_colors()
{
	color_hex_to_long(insidevercolor, insidever16);
	color_hex_to_long(insidewrongcolor, insidewrong16);
	color_hex_to_long(insidecolor, inside16);
	color_hex_to_long(ringvercolor, ringver16);
	color_hex_to_long(ringwrongcolor, ringwrong16);
	color_hex_to_long(ringcolor, ring16);
	color_hex_to_long(linecolor, line16);
	color_hex_to_long(textcolor, text16);
	color_hex_to_long(indicatorscolor, indicator16);
	color_hex_to_long(timecolor, time16);
	color_hex_to_long(datecolor, date16);
	color_hex_to_long(keyhlcolor, keyhl16);
	color_hex_to_long(bshlcolor, bshl16);
	color_hex_to_long(separatorcolor, sep16);
}
