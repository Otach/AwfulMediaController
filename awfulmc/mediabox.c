#include "mediabox.h"
#include "awfulmc.h"
#include "cairo-xlib.h"
#include "cairo.h"
#include "glib.h"
#include "pango/pango-font.h"
#include <X11/X.h>
#include <X11/Xlib.h>

#define NO_WINDOW 0

void create_window(MediaBoxContext* mbc) {
    int x = (DisplayWidth(mbc->display, mbc->screen) - WIDTH) / 2;
    int y = (DisplayHeight(mbc->display, mbc->screen) - HEIGHT) / 1.2;

    XSetWindowAttributes attrs;
    attrs.override_redirect = true;
    attrs.background_pixel = WhitePixel(mbc->display, mbc->screen);
    mbc->win = XCreateWindow(
        mbc->display,
        RootWindow(mbc->display, mbc->screen),
        x,
        y,
        WIDTH,
        HEIGHT,
        1,
        CopyFromParent,
        CopyFromParent,
        CopyFromParent,
        CWOverrideRedirect | CWBackPixel,
        &attrs
    );

    Atom wm_state = XInternAtom(mbc->display, "_NET_WM_STATE", false);
    Atom wm_state_above = XInternAtom(mbc->display, "_NET_WM_STATE_ABOVE", false);
    Atom wm_window_type = XInternAtom(mbc->display, "_NET_WM_WINDOW_TYPE", false);
    Atom wm_window_type_dialog = XInternAtom(mbc->display, "_NET_WM_WINDOW_TYPE_DIALOG", false);
    XChangeProperty(mbc->display, mbc->win, wm_window_type, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&wm_window_type_dialog, 1);
    XChangeProperty(mbc->display, mbc->win, wm_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&wm_state_above, 1);

    mbc->gc = XCreateGC(mbc->display, mbc->win, 0, NULL);
    XSelectInput(mbc->display, mbc->win, ExposureMask | ButtonPressMask);

    mbc->cairo_surface = cairo_xlib_surface_create(mbc->display, mbc->win, DefaultVisual(mbc->display, mbc->screen), WIDTH, HEIGHT);
    mbc->cairo = cairo_create(mbc->cairo_surface);

    return;
}

void destroy_window(MediaBoxContext *mbc) {
    // Remove the window, gc, cairo_surface, cairo
    cairo_destroy(mbc->cairo);
    cairo_surface_destroy(mbc->cairo_surface);
    XDestroyWindow(mbc->display, mbc->win);
    XFreeGC(mbc->display, mbc->gc);
    mbc->win = NO_WINDOW;

    return;
}

MediaBoxContext *media_box_context_new() {
    MediaBoxContext *mbc = calloc(1, sizeof(MediaBoxContext));

    mbc->display = XOpenDisplay(NULL);
    if (!mbc->display) {
        fprintf(stderr, "Failed to open X display\n");
        fflush(stderr);
        free(mbc);
        return NULL;
    }
    mbc->screen = DefaultScreen(mbc->display);


    mbc->font_large = pango_font_description_from_string("Hack 10");
    mbc->font_normal = pango_font_description_from_string("Hack 8");
    mbc->font_small = pango_font_description_from_string("Hack 6");

    mbc->shown_player_index = 0;

    // Allocate memory for buttons array
    mbc->buttons = calloc(BUTTON_COUNT, sizeof(Button*));

    int mb_center = determine_center(WIDTH, 90);
    int control_button_y = 110;

    // Initialize buttons
    mbc->buttons[BUTTON_PREVIOUS] = calloc(1, sizeof(Button));
    *mbc->buttons[BUTTON_PREVIOUS] = (Button){.x = mb_center - 100, .y = control_button_y,
                                              .width = 90, .height = 30, .label = "Prev",
                                              .border = true, .displayed = false,
                                              .on_click = send_prev};

    mbc->buttons[BUTTON_PLAY_PAUSE] = calloc(1, sizeof(Button));
    *mbc->buttons[BUTTON_PLAY_PAUSE] = (Button){.x = mb_center, .y = control_button_y,
                                                .width = 90, .height = 30, .label = "Play/Pause",
                                                .border = true, .displayed = false,
                                                .on_click = send_play_pause};

    mbc->buttons[BUTTON_NEXT] = calloc(1, sizeof(Button));
    *mbc->buttons[BUTTON_NEXT] = (Button){.x = mb_center + 100, .y = control_button_y,
                                          .width = 90, .height = 30, .label = "Next",
                                          .border = true, .displayed = false,
                                          .on_click = send_next};

    mbc->buttons[BUTTON_PLAYER_PREV] = calloc(1, sizeof(Button));
    *mbc->buttons[BUTTON_PLAYER_PREV] = (Button){.x = 0, .y = 0,
                                                 .width = 20, .height = HEIGHT, .label = "<",
                                                 .border = false, .displayed = false,
                                                 .on_click = rotate_shown_player_prev};

    mbc->buttons[BUTTON_PLAYER_NEXT] = calloc(1, sizeof(Button));
    *mbc->buttons[BUTTON_PLAYER_NEXT] = (Button){.x = WIDTH - 20, .y = 0,
                                                 .width = 20, .height = HEIGHT, .label = ">",
                                                 .border = false, .displayed = false,
                                                 .on_click = rotate_shown_player_next};

    return mbc;
}

void media_box_context_free(MediaBoxContext *mbc) {
    mbc->shown_player = NULL;
    if (mbc->win != NO_WINDOW) {
        destroy_window(mbc);
    }
    pango_font_description_free(mbc->font_large);
    pango_font_description_free(mbc->font_normal);
    pango_font_description_free(mbc->font_small);
    XCloseDisplay(mbc->display);

    if (mbc->buttons) {
        for (int i = 0; i < BUTTON_COUNT; i++) {
            free(mbc->buttons[i]);
        }
        free(mbc->buttons);
    }
    free(mbc);
}

void draw_text(MediaBoxContext *mbc, const char *text, int x, int y, FontSize font_size) {
    PangoLayout *layout = pango_cairo_create_layout(mbc->cairo);
    pango_layout_set_text(layout, text, -1);

    PangoFontDescription *font;
    switch(font_size) {
        case FONT_LARGE:
            font = mbc->font_large;
            break;
        case FONT_NORMAL:
            font = mbc->font_normal;
            break;
        case FONT_SMALL:
            font = mbc->font_small;
            break;
        default:
            font = mbc->font_normal;
    }
    pango_layout_set_font_description(layout, font);

    cairo_move_to(mbc->cairo, x, y);
    pango_cairo_show_layout(mbc->cairo, layout);

    g_object_unref(layout);
    return;
}

void draw_button(MediaBoxContext *mbc, Button *btn) {
    if (btn->border) {
        cairo_rectangle(mbc->cairo, btn->x, btn->y, btn->width, btn->height);
        cairo_stroke(mbc->cairo);
    }

    PangoLayout *layout = pango_cairo_create_layout(mbc->cairo);
    pango_layout_set_text(layout, btn->label, -1);
    pango_layout_set_font_description(layout, mbc->font_normal);

    int text_width, text_height;
    pango_layout_get_size(layout, &text_width, &text_height);

    text_width /= PANGO_SCALE;
    text_height /= PANGO_SCALE;

    int text_x = btn->x + (btn->width - text_width) / 2;
    int text_y = btn->y + (btn->height - text_height) / 2;

    cairo_move_to(mbc->cairo, text_x, text_y);
    pango_cairo_show_layout(mbc->cairo, layout);

    g_object_unref(layout);
    btn->displayed = true;
}

void draw_media_box(MediaBoxContext *mbc, GQueue *players) {
    if (mbc->win == NO_WINDOW) {
        create_window(mbc);
    }
    XClearWindow(mbc->display, mbc->win);
    cairo_set_source_rgba(mbc->cairo, 0.23, 0.23, 0.23, 1.0);
    cairo_paint(mbc->cairo);
    cairo_set_source_rgba(mbc->cairo, 1.0, 1.0, 1.0, 1.0);

    for (int i = 0; i < BUTTON_COUNT; i++) {
        mbc->buttons[i]->displayed = false;
    }

    Player *player = g_queue_peek_nth(players, mbc->shown_player_index);
    mbc->shown_player = player;
    if (player != NULL) {
        // Draw buttons
        PlayerMetadata *md = player->player_properties->metadata;
        if (player->name != NULL) {
            char *player_name = title_case(g_strdup(player->name));
            draw_text(mbc, player_name, 30, 10, FONT_SMALL);
            g_free(player_name);
        }

        if (g_queue_get_length(players) > 1) {
            draw_button(mbc, mbc->buttons[BUTTON_PLAYER_PREV]);
            draw_button(mbc, mbc->buttons[BUTTON_PLAYER_NEXT]);
        }

        if (md->title != NULL)
            draw_text(mbc, md->title, 30, 30, FONT_LARGE);

        if (md->artist != NULL)
            draw_text(mbc, md->artist, 30, 60, FONT_NORMAL);

        if (md->album != NULL)
            draw_text(mbc, md->album, 30, 80, FONT_NORMAL);

        if (player->player_properties->can_go_previous)
            draw_button(mbc, mbc->buttons[BUTTON_PREVIOUS]);

        if (player->player_properties->can_play && player->player_properties->can_pause) {
            draw_button(mbc, mbc->buttons[BUTTON_PLAY_PAUSE]);
        }

        if (player->player_properties->can_go_next)
            draw_button(mbc, mbc->buttons[BUTTON_NEXT]);

    } else {
        const char *no_players = "No Players Detected";
        draw_text(mbc, no_players, 30, 20, FONT_LARGE);
    }
    XMapWindow(mbc->display, mbc->win);
    XFlush(mbc->display);
}

void remove_media_box(MediaBoxContext *mbc) {
    destroy_window(mbc);
    XFlush(mbc->display);
}
