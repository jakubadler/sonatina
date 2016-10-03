#ifndef CLIENT_H
#define CLIENT_H

#include <mpd/client.h>

enum mpd_command {
	MPD_CMD_NONE,
	MPD_CMD_CURRENTSONG,
	MPD_CMD_IDLE,
	MPD_CMD_STATUS,
	MPD_CMD_STATS
};

struct mpd_source {
	GSource source;
	gpointer fd;
	struct mpd_async *async;
	struct mpd_parser *parser;
	GQueue pending;
};

const char *mpd_command_to_str(enum mpd_command cmd);

int client_connect(const char *host, int port);
GSource *mpd_source_new(int fd);
void mpd_source_close(GSource *source);

extern GSourceFuncs mpdsourcefuncs;

gboolean mpd_prepare(GSource *source, gint *timeout);
gboolean mpd_check(GSource *source);
gboolean mpd_dispatch(GSource *source, GSourceFunc callback, gpointer data);

gboolean mpd_parse_pair(enum mpd_command cmd, const struct mpd_pair *pair, struct mpd_status **status, struct mpd_song **song);

gboolean mpd_send(const char *cmd, ...);
gboolean mpd_send_cmd(GSource *source, enum mpd_command cmd, ...);

#endif
