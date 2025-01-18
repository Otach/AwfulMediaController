#ifndef __DWMEDIA_UTILS_H__
#define __DWMEDIA_UTILS_H__

#include "glib.h"
void g_variant_iterate_and_print(GVariant *properties);
char *title_case(char *str);
int determine_center(int bounding_size, int s);
#endif
