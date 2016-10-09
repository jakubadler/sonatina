#ifndef CLIENT_H
#define CLIENT_H

#include <glib.h>
#include <mpd/client.h>

enum mpd_cmd_type {
	MPD_CMD_NONE,
	MPD_CMD_CURRENTSONG,
	MPD_CMD_IDLE,
	MPD_CMD_STATUS,
	MPD_CMD_STATS,
	MPD_CMD_PLAY,
	MPD_CMD_STOP,
	MPD_CMD_PAUSE,
	MPD_CMD_PREV,
	MPD_CMD_NEXT,
	MPD_CMD_SETVOL,
	MPD_CMD_CLOSE
};

struct mpd_source {
	GSource source;
	gpointer fd;
	struct mpd_async *async;
	struct mpd_parser *parser;
	GQueue pending;
};

union mpd_cmd_answer {
	struct mpd_song *song; /* MPD_CMD_CURRENTSONG */
	struct mpd_status *status; /* MPD_CMD_STATUS */
	struct mpd_stats *stats; /* MPD_CMD_STATS */
	void *ptr;
	gboolean ok;
};

struct mpd_cmd {
	enum mpd_cmd_type type;
	union mpd_cmd_answer answer;
};


const char *mpd_cmd_to_str(enum mpd_cmd_type cmd);
struct mpd_cmd *mpd_cmd_new(enum mpd_cmd_type type);
void mpd_cmd_free(struct mpd_cmd *cmd);
void mpd_cmd_process_answer(struct mpd_cmd *cmd);

int client_connect(const char *host, int port);
GSource *mpd_source_new(int fd);
void mpd_source_close(GSource *source);

extern GSourceFuncs mpdsourcefuncs;

gboolean mpd_prepare(GSource *source, gint *timeout);
gboolean mpd_check(GSource *source);
gboolean mpd_dispatch(GSource *source, GSourceFunc callback, gpointer data);

gboolean mpd_parse_pair(const struct mpd_pair *pair, struct mpd_cmd *cmd);

gboolean mpd_send_cmd(GSource *source, enum mpd_cmd_type cmd, ...);

#endif
