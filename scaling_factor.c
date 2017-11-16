#include "modules.h"

double scaling_factor(void)
{
	const int dpi = (double)screen->height_in_pixels * 25.4 /
		(double)screen->height_in_millimeters;
	return (dpi / 96.0);
}
