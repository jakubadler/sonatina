#ifndef CLIENT_H
#define CLIENT_H

struct sonatina_connection {
	const char *name;
	const char *host;
	int port;
};

struct mpd_source {
	GSource source;
	gpointer fd;
	struct mpd_async *async;
};

int client_connect(const char *host, int port);
GSource *mpd_source_new(int fd);

extern GSourceFuncs mpdsourcefuncs;

gboolean mpd_prepare(GSource *source, gint *timeout);
gboolean mpd_check(GSource *source);
gboolean mpd_dispatch(GSource *source, GSourceFunc callback, gpointer data);

void mpd_send(const char *cmd, ...);

#endif
