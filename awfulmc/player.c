#include "player.h"
#include <stdint.h>
#include <stdio.h>


Player *player_new(const gchar *unique, const gchar *instance) {
    // Unique is owner
    Player *player = calloc(1, sizeof(Player));
    gchar **split = g_strsplit(instance, ".", 2);
    player->name = g_strdup(split[0]);
    g_strfreev(split);
    player->instance = g_strdup(instance);
    player->unique = g_strdup(unique);
    player->player_properties = NULL;
    return player;
}

PlayerProperties *properties_new() {
    PlayerProperties *props = calloc(1, sizeof(PlayerProperties));
    props->playback_status = PLAYBACK_STOPPED;
    props->loop_status = LOOP_NONE;
    props->rate = 1.0;
    props->shuffle = false;
    props->metadata = NULL;
    props->can_go_next = false;
    props->can_go_previous = false;
    props->can_play = false;
    props->can_pause = false;
    props->can_control = false;
    props->can_shuffle = false;
    return props;
}

PlayerMetadata *metadata_new() {
    PlayerMetadata *md = calloc(1, sizeof(PlayerMetadata));
    md->title = NULL;
    md->artist = NULL;
    md->album = NULL;
    md->art = NULL;
    return md;
}

void metadata_free(PlayerMetadata *md) {
    if (md->title != NULL) {
        g_free(md->title);
    }
    if (md->artist != NULL) {
        g_free(md->artist);
    }
    if (md->album != NULL) {
        g_free(md->album);
    }
    free(md);
}

void properties_free(PlayerProperties *props) {
    if (props->metadata != NULL) {
        metadata_free(props->metadata);
    }

    free(props);
}

void player_free(Player *player) {
    if (player == NULL) {
        return;
    }
    if (player->player_properties != NULL) {
        properties_free(player->player_properties);
    }
    if (player->unique != NULL)
        g_free(player->unique);

    g_free(player->name);
    g_free(player->instance);
    free(player);
}

bool update_player_properties(Player *player, GVariant *properties) {
    bool changed = false;
    // g_variant_iterate_and_print(properties);

    if (player->player_properties == NULL) {
        player->player_properties = properties_new();
        changed = true;
    }

    PlaybackStatus playback_status;
    gchar *status = NULL;
    if (g_variant_lookup(properties, "PlaybackStatus", "&s", &status)) {
        playback_status = convert_to_playback_status(status);
        if (playback_status != player->player_properties->playback_status) {
            player->player_properties->playback_status = playback_status;
            changed = true;
        }
    }
    status = NULL;

    LoopStatus loop_status;
    if (g_variant_lookup(properties, "LoopStatus", "&s", &status)) {
        loop_status = convert_to_loop_status(status);
        if (loop_status != player->player_properties->loop_status) {
            player->player_properties->loop_status = loop_status;
            changed = true;
        }
    }

    bool bv;
    if (g_variant_lookup(properties, "Shuffle", "b", &bv)) {
        player->player_properties->can_shuffle = true;
        if (bv != player->player_properties->shuffle) {
            player->player_properties->shuffle = bv;
            changed = true;
        }
    }

    // START METADATA STUFF
    if (player->player_properties->metadata == NULL) {
        player->player_properties->metadata = metadata_new();
    }

    GVariant *metadata = NULL, *artist_list;
    GVariantIter artist_iter;
    char *title = NULL, *artist = NULL, *album = NULL;
    metadata = g_variant_lookup_value(properties, "Metadata", (GVariantType *)"a{sv}");
    if (metadata != NULL && g_variant_get_size(metadata) > 0) {

        if (g_variant_lookup(metadata, "xesam:title", "&s", &title)) {
            if (title != NULL) {
                if (player->player_properties->metadata->title != NULL) {
                    if (g_strcmp0(title, player->player_properties->metadata->title) != 0) {
                        changed = true;
                        g_free(player->player_properties->metadata->title);
                        player->player_properties->metadata->title = g_strdup(title);
                    }
                } else {
                    player->player_properties->metadata->title = g_strdup(title);
                }
            }
        }

        if (g_variant_lookup(metadata, "xesam:album", "&s", &album)) {
            if (album != NULL && strlen(album) > 0) {
                if (player->player_properties->metadata->album != NULL) {
                    if (g_strcmp0(album, player->player_properties->metadata->album) != 0) {
                        changed = true;
                        g_free(player->player_properties->metadata->album);
                        player->player_properties->metadata->album = g_strdup(album);
                    }
                } else {
                    player->player_properties->metadata->album = g_strdup(album);
                }
            }
        }

        artist_list = g_variant_lookup_value(metadata, "xesam:artist", (GVariantType *)"as");
        if (artist_list != NULL) {
            g_variant_iter_init(&artist_iter, artist_list);
            g_variant_iter_next(&artist_iter, "&s", &artist);
            if (player->player_properties->metadata->artist != NULL) {
                if (g_strcmp0(artist, player->player_properties->metadata->artist) != 0) {
                    changed = true;
                    g_free(player->player_properties->metadata->artist);
                    player->player_properties->metadata->artist = g_strdup(artist);
                }
            } else {
                player->player_properties->metadata->artist = g_strdup(artist);
            }

            g_variant_unref(artist_list);
        }
        g_variant_unref(metadata);
    }
    // END METADATA STUFF

    if (g_variant_lookup(properties, "CanGoNext", "b", &bv)) {
        if (bv != player->player_properties->can_go_next) {
            player->player_properties->can_go_next = bv;
            changed = true;
        }
    }
    if (g_variant_lookup(properties, "CanGoPrevious", "b", &bv)) {
        if (bv != player->player_properties->can_go_previous) {
            player->player_properties->can_go_previous = bv;
            changed = true;
        }
    }
    if (g_variant_lookup(properties, "CanPlay", "b", &bv)) {
        if (bv != player->player_properties->can_play) {
            player->player_properties->can_play = bv;
            changed = true;
        }
    }
    if (g_variant_lookup(properties, "CanPause", "b", &bv)) {
        if (bv != player->player_properties->can_pause) {
            player->player_properties->can_pause = bv;
            changed = true;
        }
    }
    if (g_variant_lookup(properties, "CanControl", "b", &bv)) {
        if (bv != player->player_properties->can_control) {
            player->player_properties->can_control = bv;
            changed = true;
        }
    }

    return changed;
}

void print_player(Player *player) {

    printf(
        "Player: %s | Instance: %s | Owner: %s",
        player->name,
        player->instance,
        player->unique
    );
    if (player->player_properties == NULL) {
        // No properties were found, nothing to print
        printf("\n");
        fflush(stdout);
        return;
    }
    bool playing = player->player_properties->playback_status == PLAYBACK_PLAYING;
    PlayerMetadata *md = player->player_properties->metadata;
    printf(" | Playing: %d\n", playing);

    if (md != NULL) {
        printf("\tTitle: %s | Artist: %s | Album: %s\n", md->title, md->artist, md->album);
    }
    fflush(stdout);
}


gint player_compare(gconstpointer a, gconstpointer b) {
    Player *fn_a = (Player *)a;
    Player *fn_b = (Player *)b;
    if (fn_a->unique != NULL && fn_b->unique != NULL && strcmp(fn_a->unique, fn_b->unique) != 0) {
        return 1;
    }
    if (fn_a->name != NULL && fn_b->name != NULL &&
        strcmp(fn_a->name, fn_b->name) != 0) {
        return 1;
    }

    if (fn_a->instance != NULL && fn_b->instance != NULL && strcmp(fn_a->instance, fn_b->instance) != 0) {
        return 1;
    }

    return 0;
}
