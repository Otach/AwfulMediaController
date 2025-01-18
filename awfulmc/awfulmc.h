#ifndef __AWFULMC_H__
#define __AWFULMC_H__

#include "pango/pango-font.h"
#include "pango/pango-layout.h"
#include "player.h"
#include "utils.h"
#include "glib-object.h"
#include "mediabox.h"
#include "glib.h"
#include <X11/X.h>
#include <X11/extensions/Xrender.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <pango/pangocairo.h>
#include "mediabox.h"

#define MPRIS_PREFIX "org.mpris.MediaPlayer2."
#define SOCKET_PATH "/tmp/awfulmc.sock"


void rotate_shown_player_prev(void *data);
void rotate_shown_player_next(void *data);
void send_play_pause(void *data);
void send_prev(void *data);
void send_next(void *data);
#endif
