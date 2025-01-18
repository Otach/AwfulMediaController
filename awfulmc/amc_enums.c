#include "amc_enums.h"


PlaybackStatus convert_to_playback_status(const char *status) {
    if (status == NULL) {
        return PLAYBACK_DISABLED;
    } else if (strcmp(status, "Playing") == 0) {
        return PLAYBACK_PLAYING;
    } else if (strcmp(status, "Paused") == 0) {
        return PLAYBACK_PAUSED;
    } else if (strcmp(status, "Stopped") == 0) {
        return PLAYBACK_STOPPED;
    } else {
        g_warning("An unknown playback status string was provided (%s). Disabling playback", status);
        return PLAYBACK_DISABLED;
    }
}

LoopStatus convert_to_loop_status(const char *status) {

    if (status == NULL) {
        return LOOP_DISABLED;
    } else if (strcmp(status, "Track") == 0) {
        return LOOP_TRACK;
    } else if (strcmp(status, "Playlist") == 0) {
        return LOOP_PLAYLIST;
    } else {
        return LOOP_NONE;
    }
}
