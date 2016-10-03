#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <stdarg.h>

#include <glib.h>

#include <mpd/client.h>
#include <mpd/async.h>
#include <mpd/parser.h>

#include "client.h"
#include "core.h"
#include "util.h"

GSourceFuncs mpdsourcefuncs = {
	mpd_prepare,
	mpd_check,
	mpd_dispatch,
	NULL,
	NULL,
	NULL
};

gboolean mpd_prepare(GSource *source, gint *timeout)
{
	*timeout = -1;
	return FALSE;
}

gboolean mpd_check(GSource *source)
{
	return TRUE;
}

/**
  @brief Receive one MPD response (i.e. ending with OK or ACK line) from
  mpd source and do according actions.
  @param source MPD source
  @returns TRUE if a response was received successfully, FALSE otherwise.
  */
gboolean mpd_recv(struct mpd_source *source)
{
	char *line;
	char version[32];
	enum mpd_parser_result result;
	enum mpd_command cmd;
	struct mpd_pair pair;
	gboolean end = FALSE;

	struct mpd_status *status = NULL;
	struct mpd_song *song = NULL;

	cmd = (enum mpd_command) g_queue_peek_head(&source->pending);
	MSG_INFO("expecting answer for MPD command %s", mpd_command_to_str(cmd));

	while (!end) {
		line = mpd_async_recv_line(source->async);
		if (!line) {
			return FALSE;
		}
		MSG_DEBUG("msg recv: %s", line);

		/* ugly way to determine if this is line is the server greeting */
		if (sscanf(line, "OK MPD %s", version) == 1) {
			MSG_DEBUG("Server greeting recognized; version %s", version);
			return TRUE;
		}

		result = mpd_parser_feed(source->parser, line);
		switch (result) {
		case MPD_PARSER_MALFORMED:
			MSG_ERROR("MPD response line not understood: %s", line);
			end = TRUE;
			break;
		case MPD_PARSER_SUCCESS:
			end = TRUE;
			break;
		case MPD_PARSER_ERROR:
			MSG_ERROR("MPD error %d: %s", mpd_parser_get_server_error(source->parser), mpd_parser_get_message(source->parser));
			end = TRUE;
			break;
		case MPD_PARSER_PAIR:
			pair.name = mpd_parser_get_name(source->parser);
			pair.value = mpd_parser_get_value(source->parser);
			mpd_parse_pair(cmd, &pair, &status, &song);
			break;
		}
	}

	switch (cmd) {
	case MPD_CMD_STATUS:
		sonatina_update_status(status);
		break;
	case MPD_CMD_CURRENTSONG:
		sonatina_update_song(song);
		break;
	default:
		break;
	}

	if (status) {
		mpd_status_free(status);
	}

	if (song) {
		mpd_song_free(song);
	}

	g_queue_pop_head(&source->pending);

	return TRUE;
}

gboolean mpd_parse_pair(enum mpd_command cmd, const struct mpd_pair *pair, struct mpd_status **status, struct mpd_song **song)
{
	switch (cmd) {
	case MPD_CMD_STATUS:
		if (!(*status)) {
			*status = mpd_status_begin();
		}
		mpd_status_feed(*status, pair);
		break;
	case MPD_CMD_CURRENTSONG:
		if (!(*song)) {
			*song = mpd_song_begin(pair);
			if (!(*song)) {
				return FALSE;
			}
		} else {
			return mpd_song_feed(*song, pair);
		}
		break;
	default:
		break;
	}
	return TRUE;
}

gboolean mpd_dispatch(GSource *source, GSourceFunc callback, gpointer data)
{
	struct mpd_source *mpdsource = (struct mpd_source *) source;
	GIOCondition events;
	gboolean retval = TRUE;

	events = g_source_query_unix_fd(source, mpdsource->fd);

	if (events & G_IO_HUP) {
		MSG_DEBUG("connection closed");
		retval = FALSE;
	}
	if (events & G_IO_IN) {
		retval = retval && mpd_async_io(mpdsource->async, MPD_ASYNC_EVENT_READ);
		while (mpd_recv(mpdsource)) {};
	}
	if (events & G_IO_OUT) {
		retval = retval && mpd_async_io(mpdsource->async, MPD_ASYNC_EVENT_WRITE);
	}

	if (!retval) {
		MSG_DEBUG("mpd_dispatch(): closing connection");
	}

	return retval;
}

gboolean mpd_send(const char *cmd, ...)
{
	va_list ap;
	struct mpd_source *source = (struct mpd_source *) sonatina.mpdsource;
	gboolean retval;

	if (!source) {
		MSG_ERROR("not connected");
		return FALSE;
	}

	va_start(ap, cmd);
	MSG_DEBUG("mpd send: %s ...", cmd);
	retval = mpd_async_send_command_v(source->async, cmd, ap);
	va_end(ap);

	if (retval) {
		g_queue_push_tail(&source->pending, (gpointer) MPD_CMD_NONE);
	}

	return retval;
}

const char *mpd_command_to_str(enum mpd_command cmd)
{
	switch (cmd) {
	case MPD_CMD_CURRENTSONG:
		return "currentsong";
	case MPD_CMD_IDLE:
		return "idle";
	case MPD_CMD_STATUS:
		return "status";
	case MPD_CMD_STATS:
		return "stats";
	default:
		return NULL;
	}
}

gboolean mpd_send_cmd(GSource *source, enum mpd_command cmd, ...)
{
	va_list ap;
	struct mpd_source *mpdsource = (struct mpd_source *) source;
	const char *cmd_str;
	gboolean retval;

	cmd_str = mpd_command_to_str(cmd);
	if (!cmd_str) {
		return FALSE;
	}

	MSG_INFO("sending MPD command %s", cmd_str);

	va_start(ap, cmd);
	retval = mpd_async_send_command_v(mpdsource->async, cmd_str, ap);
	va_end(ap);

	if (retval) {
		g_queue_push_tail(&mpdsource->pending, (gpointer) cmd);
	}

	return retval;
}

/**
  @returns File descriptor of TCP connection to server.
  */
int client_connect(const char *host, int port)
{
	struct addrinfo hints;
	struct addrinfo *addr;
	int retval;
	int fd;
	char service[16];

	sprintf(service, "%d", port);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = 0;

	retval = getaddrinfo(host, service, &hints, &addr) != 0;
	if (retval != 0) {
		MSG_ERROR("Name resolution failed: %s", gai_strerror(retval));
		return -1;
	}


	fd = socket(addr->ai_family, SOCK_STREAM, 0);
	for (; addr; addr = addr->ai_next) {
		if (connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
			return fd;
		}
	}

	close(fd);

	return -1;
}

GSource *mpd_source_new(int fd)
{
	GSource *source;
	struct mpd_source *mpdsource;

	source = g_source_new(&mpdsourcefuncs, sizeof(struct mpd_source));
	mpdsource = (struct mpd_source *) source;
	mpdsource->fd = g_source_add_unix_fd(source, fd, G_IO_IN | G_IO_OUT | G_IO_HUP);
	mpdsource->async = mpd_async_new(fd);
	g_source_set_priority(source, G_PRIORITY_DEFAULT_IDLE);

	mpdsource->parser = mpd_parser_new();
	g_queue_init(&mpdsource->pending);

	return source;
}

void mpd_source_close(GSource *source)
{
	struct mpd_source *mpdsource = (struct mpd_source *) source;

	close(mpd_async_get_fd(mpdsource->async));
	mpd_async_free(mpdsource->async);
	g_source_destroy(source);
	g_source_unref(source);

	mpd_parser_free(mpdsource->parser);
}

