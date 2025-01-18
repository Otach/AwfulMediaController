#ifndef __MEDIABOX_H__
#define __MEDIABOX_H__

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include "player.h"
#include <pango/pangocairo.h>
#include <X11/extensions/Xinerama.h>

#define WIDTH 500
#define HEIGHT 150
#define BUTTON_COUNT 5

typedef enum {
    FONT_LARGE,
    FONT_NORMAL,
    FONT_SMALL,
} FontSize;

typedef enum {
    BUTTON_PREVIOUS = 0,
    BUTTON_PLAY_PAUSE = 1,
    BUTTON_NEXT = 2,
    BUTTON_PLAYER_PREV = 3,
    BUTTON_PLAYER_NEXT = 4,
} Buttons;

typedef struct {
    int x, y, width, height;
    char label[20];
    bool border;
    bool displayed;
    void (*on_click)(void *);
} Button;

typedef struct {
    Display *display;
    Window win;
    GC gc;
    int screen;
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
    PangoFontDescription *font_large;
    PangoFontDescription *font_normal;
    PangoFontDescription *font_small;
    Player *shown_player;
    int shown_player_index;
    Button **buttons;
} MediaBoxContext;

MediaBoxContext *media_box_context_new();
void media_box_context_free(MediaBoxContext *mbc);
void draw_media_box(MediaBoxContext *mbc, GQueue *players);
void remove_media_box(MediaBoxContext *mbc);
#endif
