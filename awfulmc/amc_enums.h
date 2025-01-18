#ifndef __DW_ENUMS_H__
#define __DW_ENUMS_H__
#include <gio/gio.h>

typedef enum {
    PLAYBACK_PLAYING = 0,
    PLAYBACK_PAUSED = 1,
    PLAYBACK_STOPPED = 2,
    PLAYBACK_DISABLED = 3,
} PlaybackStatus;

typedef enum {
    LOOP_NONE = 0,
    LOOP_TRACK = 1,
    LOOP_PLAYLIST = 2,
    LOOP_DISABLED = -1
} LoopStatus;

PlaybackStatus convert_to_playback_status(const char *status);
LoopStatus convert_to_loop_status(const char *status);
#endif
