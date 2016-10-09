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

const char *mpd_cmd_to_str(enum mpd_cmd_type cmd)
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
	case MPD_CMD_CLOSE:
		return "close";
	case MPD_CMD_PLAY:
		return "play";
	case MPD_CMD_STOP:
		return "stop";
	case MPD_CMD_PAUSE:
		return "pause";
	case MPD_CMD_PREV:
		return "previous";
	case MPD_CMD_NEXT:
		return "next";
	case MPD_CMD_SETVOL:
		return "setvol";
	default:
		return NULL;
	}
}

struct mpd_cmd *mpd_cmd_new(enum mpd_cmd_type type)
{
	struct mpd_cmd *cmd;

	cmd = g_malloc(sizeof(struct mpd_cmd));
	cmd->type = type;
	cmd->answer.ptr = NULL;

	return cmd;
}

void mpd_cmd_free(struct mpd_cmd *cmd)
{
	if (!cmd) {
		return;
	}

	switch (cmd->type) {
	case MPD_CMD_STATUS:
		if (cmd->answer.status) {
			mpd_status_free(cmd->answer.status);
		}
		break;
	case MPD_CMD_CURRENTSONG:
		if (cmd->answer.song) {
			mpd_song_free(cmd->answer.song);
		}
		break;
	default:
		break;
	}

	g_free(cmd);
}

void mpd_cmd_process_answer(struct mpd_cmd *cmd)
{
	if (!cmd) {
		return;
	}

	switch (cmd->type) {
	case MPD_CMD_STATUS:
		sonatina_update_status(cmd->answer.status);
		break;
	case MPD_CMD_CURRENTSONG:
		sonatina_update_song(cmd->answer.song);
		break;
	default:
		break;
	}
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
	struct mpd_cmd *cmd;
	struct mpd_pair pair;
	gboolean end = FALSE;
	gboolean success = FALSE;

	cmd = g_queue_peek_head(&source->pending);
	if (!cmd) {
		MSG_ERROR("received answer while no command pending");
		return FALSE;
	}
	MSG_INFO("expecting answer for MPD command %s", mpd_cmd_to_str(cmd->type));

	while (!end) {
		line = mpd_async_recv_line(source->async);
		if (!line) {
			return FALSE;
		}
		MSG_DEBUG("msg recv: %s", line);

		/* ugly way to determine if this is line is the server greeting */
		if (sscanf(line, "OK MPD %s", version) == 1) {
			MSG_DEBUG("Server greeting recognized; version %31s", version);
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
			success = TRUE;
			break;
		case MPD_PARSER_ERROR:
			MSG_ERROR("MPD error %d: %s", mpd_parser_get_server_error(source->parser), mpd_parser_get_message(source->parser));
			end = TRUE;
			break;
		case MPD_PARSER_PAIR:
			pair.name = mpd_parser_get_name(source->parser);
			pair.value = mpd_parser_get_value(source->parser);
			mpd_parse_pair(&pair, cmd);
			break;
		}
	}

	if (success) {
		mpd_cmd_process_answer(cmd);
	}
	mpd_cmd_free(cmd);
	g_queue_pop_head(&source->pending);

	if (g_queue_is_empty(&source->pending)) {
		mpd_send_cmd((GSource *) source, MPD_CMD_IDLE, NULL);
	}

	return TRUE;
}

/**
  @param cmd Type of MPD command which this pair belongs to.
  @param pair MPD pair to be parsed.
  @return TRUE if pair is successfully parsed (is valid for given command type).
  */
gboolean mpd_parse_pair(const struct mpd_pair *pair, struct mpd_cmd *cmd)
{
	gboolean result = TRUE;

	switch (cmd->type) {
	case MPD_CMD_STATUS:
		if (!(cmd->answer.status)) {
			cmd->answer.status = mpd_status_begin();
		}
		mpd_status_feed(cmd->answer.status, pair);
		break;
	case MPD_CMD_CURRENTSONG:
		if (!(cmd->answer.song)) {
			cmd->answer.song = mpd_song_begin(pair);
			if (!(cmd->answer.song)) {
				result = FALSE;
			}
		} else {
			result = mpd_song_feed(cmd->answer.song, pair);
		}
		break;
	case MPD_CMD_IDLE:
		if (strcmp(pair->name, "changed")) {
			break;
		}
		MSG_DEBUG("changed: %s", pair->value);
		break;
	default:
		break;
	}

	return result;
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

/**
  @param ... List of arguments to command terminated with NULL.
  @return TRUE when command was sent, FALSE otherwise.
  */
gboolean mpd_send_cmd(GSource *source, enum mpd_cmd_type type, ...)
{
	va_list ap;
	struct mpd_source *mpdsource = (struct mpd_source *) source;
	const char *cmd_str;
	struct mpd_cmd *cmd;
	struct mpd_cmd *pending;
	gboolean retval;

	if (!source) {
		MSG_WARNING("invalid mpd source");
		return FALSE;
	}

	cmd_str = mpd_cmd_to_str(type);
	if (!cmd_str) {
		MSG_WARNING("invalid command");
		return FALSE;
	}

	cmd = mpd_cmd_new(type);
	pending = g_queue_peek_tail(&mpdsource->pending);

	if (pending && pending->type == MPD_CMD_IDLE) {
		MSG_DEBUG("stop idling");
		mpd_async_send_command(mpdsource->async, "noidle", NULL);
	}

	MSG_INFO("sending MPD command %s", cmd_str);

	va_start(ap, type);
	retval = mpd_async_send_command_v(mpdsource->async, cmd_str, ap);
	va_end(ap);

	if (retval) {
		g_queue_push_tail(&mpdsource->pending, cmd);
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
	struct mpd_cmd *cmd;

	close(mpd_async_get_fd(mpdsource->async));
	mpd_async_free(mpdsource->async);

	mpd_parser_free(mpdsource->parser);

	while (!g_queue_is_empty(&mpdsource->pending)) {
		cmd = g_queue_pop_head(&mpdsource->pending);
		mpd_cmd_free(cmd);
	}
		
	g_source_destroy(source);
	g_source_unref(source);
}

