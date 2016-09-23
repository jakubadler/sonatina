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

struct mpd_parser *parser = NULL;

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

gboolean mpd_recv(struct mpd_source *source)
{
	char *line;
	enum mpd_parser_result status;

	mpd_async_io(source->async, MPD_ASYNC_EVENT_READ);
	line = mpd_async_recv_line(source->async);
	printf("mpd recv: %s\n", line);
	if (!line) {
		puts("null line");
		return FALSE;
	}

	if (!parser) {
		parser = mpd_parser_new();
		if (!parser) {
			fprintf(stderr, "Couldn't allocate parser\n");
			return FALSE;
		}
	}

	status = mpd_parser_feed(parser, line);
	switch (status) {
	case MPD_PARSER_MALFORMED:
		puts("malformed");
		break;
	case MPD_PARSER_SUCCESS:
		puts("OK");
		break;
	case MPD_PARSER_ERROR:
		puts("error");
		break;
	case MPD_PARSER_PAIR:
		puts("pair");
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
		puts("connection closed");
		retval = retval && FALSE;
	}
	if (events & G_IO_IN) {
		retval = retval && mpd_recv(mpdsource);
	}
	if (events & G_IO_OUT) {
		retval = retval && mpd_async_io(mpdsource->async, MPD_ASYNC_EVENT_WRITE);
	}

	return retval;
}

void mpd_send(const char *cmd, ...)
{
	va_list ap;
	struct mpd_source *source = (struct mpd_source *) sonatina.mpdsource;

	if (!source) {
		fprintf(stderr, "not connected\n");
		return;
	}

	va_start(ap, cmd);
	mpd_async_send_command_v(source->async, cmd, ap);
	va_end(ap);
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
		fprintf(stderr, "Name resolution failed: %s\n", gai_strerror(retval));
		return -1;
	}


	for (; addr; addr = addr->ai_next) {
		fd = socket(addr->ai_family, SOCK_STREAM, 0);
		if (connect(fd, addr->ai_addr, addr->ai_addrlen) == 0) {
			return fd;
		}
	}

	return -1;
}

GSource *mpd_source_new(int fd)
{
	GSource *source;

	source = g_source_new(&mpdsourcefuncs, sizeof(struct mpd_source));
	((struct mpd_source *) source)->fd = g_source_add_unix_fd(source, fd, G_IO_IN | G_IO_OUT | G_IO_HUP);
	((struct mpd_source *) source)->async = mpd_async_new(fd);
	g_source_set_priority(source, G_PRIORITY_DEFAULT_IDLE);

	return source;
}

