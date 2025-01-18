#include "utils.h"
#include <math.h>

void g_variant_iterate_and_print(GVariant *properties) {
    GVariantIter iter;
    gchar *key;
    GVariant *value;

    g_variant_iter_init(&iter, properties);
    while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
        gchar *value_str = g_variant_print(value, FALSE);
        g_print("Property: %s, Value: %s\n", key, value_str);

        g_free(value_str);
        g_free(key);
        g_variant_unref(value);
    }
}

char *title_case(char *str) {
    if (str == NULL) {
        return str;
    }

    if (str[0] >= 0x61 && str[0] <= 0x7a) {
        str[0] = str[0] - 0x20;
        return str;
    }
    return str;
}

int determine_center(int bounding_size, int s) {
    return (floor(bounding_size / 2.0) - floor(s / 2.0) - 1);
}
