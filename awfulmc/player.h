#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <stdbool.h>
#include <stdint.h>
#include "amc_enums.h"

typedef struct {
    char *title;
    char *artist;
    char *album;
    uint8_t *art;
} PlayerMetadata;

typedef struct {
    PlaybackStatus playback_status;
    LoopStatus loop_status;
    double rate;
    bool shuffle;
    bool can_go_next;
    bool can_go_previous;
    bool can_play;
    bool can_pause;
    bool can_control;
    bool can_shuffle;
    PlayerMetadata *metadata;
} PlayerProperties;

typedef struct {
    char *unique;
    char *name;
    char *instance;
    PlayerProperties *player_properties;
} Player;

Player *player_new(const gchar *unique, const gchar *instance);
PlayerProperties *properties_new();
PlayerMetadata *metadata_new();
void metadata_free(PlayerMetadata *md);
void properties_free(PlayerProperties *props);
void player_free(Player *player);
bool update_player_properties(Player *player, GVariant *properties);
void print_player(Player *player);
gint player_compare(gconstpointer a, gconstpointer b);
#endif
