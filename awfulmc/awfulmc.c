#include "awfulmc.h"

GMainLoop *main_loop;

typedef struct {
    GDBusConnection *con;
    GQueue *players;
    GQueue *pending_players;
    Player *pending_active;
    int display_fd;
    bool media_box_visible;
    MediaBoxContext *mbc;
} AwfulMCContext;

void handle_media_box(AwfulMCContext *ctx) {
    g_debug("media_box_visible=%d", ctx->media_box_visible);
    if (ctx->media_box_visible) {
        draw_media_box(ctx->mbc, ctx->players);
    } else {
        remove_media_box(ctx->mbc);
        ctx->mbc->shown_player = NULL;
        ctx->mbc->shown_player_index = 0;
    }
}

gboolean get_player_properties(AwfulMCContext *ctx, Player* player) {
    GVariant *reply, *properties;
    GError *err = NULL;

    reply = g_dbus_connection_call_sync(
        ctx->con,
        player->unique,
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", "org.mpris.MediaPlayer2.Player"),
        NULL,
        G_DBUS_CALL_FLAGS_NO_AUTO_START,
        -1,
        NULL,
        &err
    );

    if (err != NULL) {
        g_printerr("Could not get player properties for player %s: %s", player->name, err->message);
        g_error_free(err);
        return FALSE;
    }

    properties = g_variant_get_child_value(reply, 0);
    update_player_properties(player, properties);
    g_variant_unref(properties);
    g_variant_unref(reply);
    return TRUE;
}

const gchar *get_player_owner(GDBusConnection *con, const gchar *name) {
    GError *error = NULL;
    GVariant *owner_reply = g_dbus_connection_call_sync(
        con,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "GetNameOwner",
        g_variant_new("(s)", name),
        NULL,
        G_DBUS_CALL_FLAGS_NO_AUTO_START,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning("could not get owner for name %s: %s", name, error->message);
        g_error_free(error);
        return NULL;
    }

    GVariant *owner_reply_value = g_variant_get_child_value(owner_reply, 0);
    const gchar *owner = g_strdup(g_variant_get_string(owner_reply_value, 0));
    g_debug("Found owner for %s: %s", name, owner);

    g_variant_unref(owner_reply_value);
    g_variant_unref(owner_reply);
    return owner;
}

GQueue *get_players(AwfulMCContext *ctx, GError **err) {
    GQueue *players = g_queue_new();
    GError *tmp_error = NULL;

    GDBusProxy *proxy = g_dbus_proxy_new_sync(
        ctx->con,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        NULL,
        &tmp_error
    );

    if (tmp_error != NULL) {
        g_warning("Could not create the proxy when getting player names");
        g_propagate_error(err, tmp_error);
        return NULL;
    }

    g_info("Getting list of player names from D-Bus");
    GVariant *reply = g_dbus_proxy_call_sync(
        proxy,
        "ListNames",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &tmp_error
    );

    if (tmp_error != NULL) {
        g_object_unref(proxy);
        g_propagate_error(err, tmp_error);
        return NULL;
    }

    GVariant *reply_child = g_variant_get_child_value(reply, 0);
    gsize reply_count;
    const gchar **names = g_variant_get_strv(reply_child, &reply_count);

    size_t offset = strlen(MPRIS_PREFIX);
    for (gsize i = 0; i < reply_count; i++) {
        if (g_str_has_prefix(names[i], MPRIS_PREFIX)) {

            const gchar *owner = get_player_owner(ctx->con, names[i]);
            if (owner == NULL) {
                g_warning("Discarding player %s because we could not get owner", names[i]);
                continue;
            }

            Player *player = player_new(owner, names[i]+offset);

            if (!get_player_properties(ctx, player)) {
                g_warning("Discarding player %s because we could not get the properties", player->name);
                player_free(player);
                continue;
            }

            g_queue_push_head(players, player);
        }
    }
    g_info("Found %u players on the bus.", g_queue_get_length(players));

    g_object_unref(proxy);
    g_variant_unref(reply);
    g_variant_unref(reply_child);
    g_free(names);
    return players;
}

static Player *context_find_player(AwfulMCContext *ctx, const char *unique, const char *name, const char *instance) {
    const Player find_name = {
        .unique = (char *)unique,
        .name = (char *)name,
        .instance = (char *)instance,
    };
    GList *found = g_queue_find_custom(ctx->players, &find_name, player_compare);
    if (found != NULL) {
        return (Player *)found->data;
    }

    found = g_queue_find_custom(ctx->pending_players, &find_name, player_compare);
    if (found != NULL) {
        return (Player *)found->data;
    }

    return NULL;
}

void print_players(AwfulMCContext *ctx) {
    for (guint i = 0; i < g_queue_get_length(ctx->players); i++) {
        Player *player = g_queue_peek_nth(ctx->players, i);
        print_player(player);
        printf("\n");
    }
    fflush(stdout);
}

void rotate_shown_player_prev(void *data) {
    AwfulMCContext *ctx = data;
    guint player_count = g_queue_get_length(ctx->players);
    if (player_count < 1)
        return;

    ctx->mbc->shown_player_index = (ctx->mbc->shown_player_index - 1) % player_count;
}

void rotate_shown_player_next(void *data) {
    AwfulMCContext *ctx = data;
    guint player_count = g_queue_get_length(ctx->players);
    if (player_count < 1)
        return;

    ctx->mbc->shown_player_index = (ctx->mbc->shown_player_index + 1) % player_count;
}

void send_mpris_command(AwfulMCContext *ctx, const char *instance, const char *command) {
    char service_name[256];
    snprintf(service_name, sizeof(service_name), "org.mpris.MediaPlayer2.%s", instance);

    GDBusMessage *msg = g_dbus_message_new_method_call(service_name, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", command);
    g_dbus_connection_send_message(ctx->con, msg, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
}

void send_play_pause(void *data) {
    AwfulMCContext *ctx = data;
    send_mpris_command(ctx, ctx->mbc->shown_player->instance, "PlayPause");
}

void send_prev(void *data) {
    AwfulMCContext *ctx = data;
    send_mpris_command(ctx, ctx->mbc->shown_player->instance, "Previous");
}

void send_next(void *data) {
    AwfulMCContext *ctx = data;
    send_mpris_command(ctx, ctx->mbc->shown_player->instance, "Next");
}

static gboolean media_box_callback(GIOChannel *source, GIOCondition condition, gpointer user_data) {
    AwfulMCContext *ctx = (AwfulMCContext *)user_data;

    while (XPending(ctx->mbc->display) > 0) {
        XEvent event;
        XNextEvent(ctx->mbc->display, &event);

        if (event.type == Expose) {
            draw_media_box(ctx->mbc, ctx->players);
        } else if (event.type == ButtonPress) {
            XButtonEvent *button_event = (XButtonEvent *)&event;
            for (int i = 0; i < BUTTON_COUNT; i++) {
                Button *btn = ctx->mbc->buttons[i];
                if (!btn->displayed)
                    continue;

                if (button_event->x >= btn->x && button_event->x <= (btn->x + btn->width) && button_event->y >= btn->y && button_event->y <= (btn->y + btn->height)) {
                    g_info("Button %s clicked.", btn->label);
                    if (btn->on_click) {
                        btn->on_click(ctx);
                        draw_media_box(ctx->mbc, ctx->players);
                    }
                    break;
                }
            }
        }
    }

    return true;
}

void player_signal_proxy_callback(GDBusConnection *con, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data) {
    AwfulMCContext *ctx = user_data;
    Player *player = context_find_player(ctx, sender_name, NULL, NULL);
    bool changed = false;

    if (player == NULL)
        return;

    if (g_strcmp0(interface_name, "org.mpris.MediaPlayer2.Player") != 0 && g_strcmp0(interface_name, "org.freedesktop.DBus.Properties") != 0)
        return;

    gchar *p = g_variant_print(parameters, true);
    g_debug("got player signal: sender=%s, object_path=%s, interface_name=%s, signal_name=%s, parameters=%s", sender_name, object_path, interface_name, signal_name, p);
    g_free(p);

    if (player == ctx->pending_active)
        return;

    if (g_strcmp0(signal_name, "PropertiesChanged") == 0) {
        GVariant *properties = g_variant_get_child_value(parameters, 1);
        changed = update_player_properties(player, properties);
        g_variant_unref(properties);
    }

    if (changed) {
        g_info("Player %s Properties Changed", player->name);
        if (player == ctx->mbc->shown_player) {
            handle_media_box(ctx);
        }
    }

    return;
}

void name_owner_changed_signal_callback(GDBusConnection *con, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data) {
    AwfulMCContext *ctx = user_data;
    GVariant *name_variant = g_variant_get_child_value(parameters, 0);
    GVariant *new_owner_variant = g_variant_get_child_value(parameters, 2);
    const gchar *name = g_variant_get_string(name_variant, 0);
    const gchar *new_owner = g_variant_get_string(new_owner_variant, 0);

    if (!g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
        g_variant_unref(name_variant);
        g_variant_unref(new_owner_variant);
        return;
    }

    g_debug("got name owner changed signal: name='%s', 'owner'%s'", name, new_owner);

    gsize name_offset = strlen(MPRIS_PREFIX);
    if (strlen(new_owner) > 0) {
        g_debug("player name appeared: unique=%s, name=%s", new_owner, name);
        Player *player = context_find_player(ctx, NULL, NULL, name+name_offset);
        if (player != NULL) {
            g_debug("player already managed, updating name");
            g_free(player->unique);
            player->unique = g_strdup(name);
            g_variant_unref(name_variant);
            g_variant_unref(new_owner_variant);
            return;
        }

        g_debug("getting properties for new player");
        player = player_new(new_owner, name+name_offset);
        g_queue_remove(ctx->players, player);
        g_queue_remove(ctx->pending_players, player);
        g_queue_push_tail(ctx->pending_players, player);
        ctx->pending_active = player;
        get_player_properties(ctx, player);
        g_queue_remove(ctx->pending_players, player);
        g_queue_push_tail(ctx->players, player);
        ctx->pending_active = NULL;
    } else {
        Player *player = context_find_player(ctx, NULL, NULL, name+name_offset);
        if (player == NULL) {
            g_debug("name '%s' not found in queue", name+name_offset);
            g_variant_unref(name_variant);
            g_variant_unref(new_owner_variant);
            return;
        }

        g_debug("removing name from players: unique=%s, name=%s", player->unique, player->name);
        g_queue_remove(ctx->players, player);
        g_queue_remove(ctx->pending_players, player);
        if (ctx->pending_active == player) {
            ctx->pending_active = NULL;
        }
        if (ctx->mbc->shown_player == player) {
            if (ctx->mbc->shown_player_index > 0) {
                ctx->mbc->shown_player_index--;
            } else {
                ctx->mbc->shown_player_index = 0;
            }
            ctx->mbc->shown_player = NULL;
            handle_media_box(ctx);
        }
        player_free(player);
    }

    handle_media_box(ctx);

    g_variant_unref(name_variant);
    g_variant_unref(new_owner_variant);
    return;
}

void handle_exit_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_debug("Got a SIGINT or SIGTERM");
        g_main_loop_quit(main_loop);
    }
}

int create_unix_socket() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket creation failed");
        exit(-1);
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("socket bind failed");
        close(fd);
        exit(-1);
    }

    if (listen(fd, 1) == -1) {
        perror("socket listen failed");
        close(fd);
        exit(-1);
    }
    return fd;
}

gboolean unix_socket_callback(GIOChannel *channel, GIOCondition condition, gpointer user_data) {
    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        fprintf(stderr, "Socket error or hangup\n");
        g_io_channel_unref(channel);
        return false;;
    }

    AwfulMCContext *ctx = (AwfulMCContext *)user_data;

    char buffer[256] = {0};
    gsize bytes_read;
    GIOStatus status = g_io_channel_read_chars(channel, buffer, sizeof(buffer) - 1, &bytes_read, NULL);

    if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (strcmp(buffer, "TOGGLE\n") == 0) {
            g_info("received TOGGLE command");
            ctx->media_box_visible = !ctx->media_box_visible;
            handle_media_box(ctx);
        } else if (strcmp(buffer, "PLAYPAUSE\n") == 0) {
            if (ctx->media_box_visible && ctx->mbc->shown_player != NULL) {
                send_play_pause(ctx);
            }
        } else if (strcmp(buffer, "ROTATE\n") == 0) {
            if (ctx->media_box_visible) {
                rotate_shown_player_next(ctx);
                handle_media_box(ctx);
            }
        } else if (strcmp(buffer, "PREVIOUS\n") == 0) {
            if (ctx->media_box_visible && ctx->mbc->shown_player != NULL) {
                send_prev(ctx);
            }
        } else if (strcmp(buffer, "NEXT\n") == 0) {
            if (ctx->media_box_visible && ctx->mbc->shown_player != NULL) {
                send_next(ctx);
            }
        } else {
            fprintf(stderr, "Unknown command: %s\n", buffer);
        }
    }

    int client_fd = g_io_channel_unix_get_fd(channel);
    close(client_fd);
    g_io_channel_unref(channel);
    return false;
}

gboolean accept_client_connection(GIOChannel *channel, GIOCondition condition, gpointer user_data) {
    int fd = g_io_channel_unix_get_fd(channel);
    int client = accept(fd, NULL, NULL);
    if (client == -1) {
        perror("Could not accept client connection");
        return true;
    }

    GIOChannel *client_channel = g_io_channel_unix_new(client);
    g_io_channel_set_flags(client_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(client_channel, G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL, unix_socket_callback, user_data);

    return true;
}

int main(void) {

    AwfulMCContext ctx = {0};
    GError *err = NULL;

    ctx.media_box_visible = false;
    ctx.mbc = media_box_context_new();

    int fd = create_unix_socket();
    GIOChannel *server_channel = g_io_channel_unix_new(fd);
    g_io_channel_set_flags(server_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch(server_channel, G_IO_IN, accept_client_connection, &ctx);

    ctx.con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);
    if (err != NULL) {
        g_printerr("could not connect to session message bus: %s", err->message);
        return -1;
    }

    g_debug("connected to dbus: %s", g_dbus_connection_get_unique_name(ctx.con));

    ctx.players = get_players(&ctx, &err);
    ctx.pending_players = g_queue_new();
    if (err != NULL) {
        g_printerr("Could not list currently active players: %s", err->message);
        g_object_unref(ctx.con);
        return -1;
    }

    ctx.display_fd = ConnectionNumber(ctx.mbc->display);
    GIOChannel *channel = g_io_channel_unix_new(ctx.display_fd);
    g_io_add_watch(channel, G_IO_IN, media_box_callback, &ctx);

    main_loop = g_main_loop_new(NULL, false);

    g_dbus_connection_signal_subscribe(
        ctx.con,
        "org.freedesktop.DBus",
        "org.freedesktop.DBus",
        "NameOwnerChanged",
        "/org/freedesktop/DBus",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        name_owner_changed_signal_callback,
        &ctx,
        NULL);

    g_dbus_connection_signal_subscribe(
        ctx.con,
        NULL,
        NULL,
        NULL,
        "/org/mpris/MediaPlayer2",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        player_signal_proxy_callback,
        &ctx,
        NULL);

    print_players(&ctx);

    signal(SIGINT, handle_exit_signal);
    signal(SIGTERM, handle_exit_signal);
    g_main_loop_run(main_loop);
    g_main_loop_unref(main_loop);
    g_io_channel_unref(channel);
    g_io_channel_unref(server_channel);
    close(fd);
    unlink(SOCKET_PATH);

    g_queue_free_full(ctx.players, (GDestroyNotify)player_free);
    media_box_context_free(ctx.mbc);
    g_object_unref(ctx.con);
    return 0;
}
