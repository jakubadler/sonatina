#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

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

gboolean mpd_dispatch(GSource *source, GSourceFunc callback, gpointer data)
{
	struct mpd_source *mpdsource = (struct mpd_source *) source;
	GIOCondition events, revents;
	enum mpd_async_event mpd_events;
	gboolean retval = TRUE;

	mpd_events = mpd_async_events(mpdsource->async);
	events = 0;

	if (mpd_events & MPD_ASYNC_EVENT_READ)
		events = events | G_IO_IN;
	if (mpd_events & MPD_ASYNC_EVENT_WRITE)
		events = events | G_IO_OUT;
	if (mpd_events & MPD_ASYNC_EVENT_HUP)
		events = events | G_IO_HUP;
	if (mpd_events & MPD_ASYNC_EVENT_ERROR)
		events = events | G_IO_ERR;

	g_source_modify_unix_fd(source, mpdsource->fd, events);
	revents = g_source_query_unix_fd(source, mpdsource->fd);

	if (revents & G_IO_IN) {
		retval = retval && mpd_async_io(mpdsource->async, MPD_ASYNC_EVENT_READ);
		while (mpd_recv(mpdsource)) {};
	}
	if (revents & G_IO_OUT) {
		retval = retval && mpd_async_io(mpdsource->async, MPD_ASYNC_EVENT_WRITE);
	}
	if (revents & G_IO_HUP) {
		MSG_DEBUG("connection closed");
		retval = FALSE;
	}

	if (!retval) {
		MSG_DEBUG("mpd_dispatch(): closing connection");
	}

	return retval;
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
	case MPD_CMD_SEEKCUR:
		return "seekcur";
	case MPD_CMD_SETVOL:
		return "setvol";
	case MPD_CMD_PLINFO:
		return "playlistinfo";
	case MPD_CMD_LIST: return "list";
	case MPD_CMD_LSINFO:
		return "lsinfo";
	case MPD_CMD_FIND:
		return "find";
	case MPD_CMD_ADD:
		return "add";
	case MPD_CMD_FINDADD:
		return "findadd";
	case MPD_CMD_CLEAR:
		return "clear";
	case MPD_CMD_UPDATE:
		return "update";
	case MPD_CMD_DELETE:
		return "delete";
	case MPD_CMD_DELETEID:
		return "deleteid";
	case MPD_CMD_LISTPL:
		return "listplaylist";
	case MPD_CMD_LISTPLINFO:
		return "listplaylistinfo";
	case MPD_CMD_LISTPLS:
		return "listplaylists";
	case MPD_CMD_LOAD:
		return "load";
	case MPD_CMD_RM:
		return "rm";
	default:
		return NULL;
	}
}

struct mpd_cmd *mpd_cmd_new(enum mpd_cmd_type type)
{
	struct mpd_cmd *cmd;

	cmd = g_malloc(sizeof(struct mpd_cmd));
	cmd->args = NULL;
	cmd->type = type;
	cmd->parse_pair = NULL;
	cmd->process = NULL;
	cmd->free_answer = NULL;

	switch (type) {
	case MPD_CMD_CURRENTSONG:
		cmd->parse_pair = parse_pair_song;
		cmd->answer.song = NULL;
		break;
	case MPD_CMD_STATUS:
		cmd->parse_pair = parse_pair_status;
		cmd->answer.status = NULL;
		break;
	case MPD_CMD_STATS:
		cmd->answer.stats = NULL;
		break;
	case MPD_CMD_IDLE:
		cmd->parse_pair = parse_pair_idle;
		cmd->process = cmd_process_idle;
		cmd->answer.idle = 0;
		break;
	case MPD_CMD_PLINFO:
		cmd->parse_pair = parse_pair_plsong;
		cmd->process = cmd_process_plinfo;
		cmd->answer.plinfo.song = NULL;
		cmd->answer.plinfo.list = NULL;
		break;
	case MPD_CMD_LSINFO:
	case MPD_CMD_FIND:
	case MPD_CMD_LISTPL:
	case MPD_CMD_LISTPLINFO:
	case MPD_CMD_LISTPLS:
		cmd->parse_pair = parse_pair_lsinfo;
		cmd->process = cmd_process_lsinfo;
		cmd->answer.lsinfo.entity = NULL;
		cmd->answer.lsinfo.list = NULL;
		break;
	case MPD_CMD_LIST:
		cmd->parse_pair = parse_pair_list;
		cmd->process = cmd_process_list;
		cmd->answer.list.entity = NULL;
		cmd->answer.list.list = NULL;
		break;
	default:
		break;
	}

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
	case MPD_CMD_PLINFO:
		g_list_free_full(cmd->answer.plinfo.list, (GDestroyNotify) mpd_song_free);
		break;
	case MPD_CMD_LSINFO:
		g_list_free_full(cmd->answer.lsinfo.list, (GDestroyNotify) mpd_entity_free);
		break;
	case MPD_CMD_LIST:
		g_list_free_full(cmd->answer.list.list, (GDestroyNotify) mpd_tag_entity_free);
		break;
	default:
		break;
	}

	g_list_free_full(cmd->args, g_free);

	g_free(cmd);
}

void cmd_process_idle(union mpd_cmd_answer *answer)
{
	if (answer->idle & MPD_CHANGED_PLAYER ||
	    answer->idle & MPD_CHANGED_MIXER ||
	    answer->idle & MPD_CHANGED_OPTIONS ) {
		mpd_send(sonatina.mpdsource, MPD_CMD_STATUS, NULL);
	}
	if (answer->idle & MPD_CHANGED_PL) {
		mpd_send(sonatina.mpdsource, MPD_CMD_PLINFO, NULL);
	}
	if (answer->idle & MPD_CHANGED_PLAYER) {
		mpd_send(sonatina.mpdsource, MPD_CMD_CURRENTSONG, NULL);
	}
	if (answer->idle & MPD_CHANGED_STORED_PL) {
		mpd_send(sonatina.mpdsource, MPD_CMD_LISTPLS, NULL);
	}
}

void cmd_process_plinfo(union mpd_cmd_answer *answer)
{
	if (answer->plinfo.song) {
		answer->plinfo.list = g_list_prepend(answer->plinfo.list, answer->plinfo.song);
	}
	answer->plinfo.list = g_list_reverse(answer->plinfo.list);
}

void cmd_process_lsinfo(union mpd_cmd_answer *answer)
{
	if (answer->lsinfo.entity) {
		answer->lsinfo.list = g_list_prepend(answer->lsinfo.list, answer->lsinfo.entity);
	}
	answer->lsinfo.list = g_list_reverse(answer->lsinfo.list);
}

void cmd_process_list(union mpd_cmd_answer *answer)
{
	if (answer->list.entity) {
		answer->list.list = g_list_prepend(answer->list.list, answer->list.entity);
	}
	answer->list.list = g_list_reverse(answer->list.list);
}

gboolean parse_pair_status(union mpd_cmd_answer *answer, const struct mpd_pair *pair)
{
	if (!(answer->status)) {
		answer->status = mpd_status_begin();
		if (!(answer->status)) {
			MSG_ERROR("Couldn't allocate mpd status");
			return FALSE;
		}
	}
	mpd_status_feed(answer->status, pair);

	return TRUE;
}

gboolean parse_pair_song(union mpd_cmd_answer *answer, const struct mpd_pair *pair)
{
	if (answer->song) {
		return mpd_song_feed(answer->song, pair);
	}
	answer->song = mpd_song_begin(pair);
	if (!(answer->song)) {
		return FALSE;
	}

	return TRUE;

}

gboolean parse_pair_plsong(union mpd_cmd_answer *answer, const struct mpd_pair *pair)
{
	if (!answer->plinfo.song) {
		/* first pair of a song*/
		answer->plinfo.song = mpd_song_begin(pair);
		if (!answer->plinfo.song) {
			MSG_ERROR("couldn't allocate song");
			return FALSE;
		}
	} else if (!mpd_song_feed(answer->plinfo.song, pair)) {
		/* beginning of a new song */
		answer->plinfo.list = g_list_prepend(answer->plinfo.list, answer->plinfo.song);
		answer->plinfo.song = mpd_song_begin(pair);
		if (!answer->plinfo.song) {
			return FALSE;
		}
	}

	return TRUE;
}

gboolean parse_pair_lsinfo(union mpd_cmd_answer *answer, const struct mpd_pair *pair)
{
	if (!answer->lsinfo.entity) {
		/* first pair of a directory */
		answer->lsinfo.entity = mpd_entity_begin(pair);
		if (!answer->lsinfo.entity) {
			MSG_ERROR("couldn't allocate directory");
			return FALSE;
		}
	} else if (!mpd_entity_feed(answer->lsinfo.entity, pair)) {
		/* beginning of a new directory */
		answer->lsinfo.list = g_list_prepend(answer->lsinfo.list, answer->lsinfo.entity);
		answer->lsinfo.entity = mpd_entity_begin(pair);
		if (!answer->lsinfo.entity) {
			return FALSE;
		}
	}

	return TRUE;
}

gboolean parse_pair_list(union mpd_cmd_answer *answer, const struct mpd_pair *pair)
{
	if (!answer->list.entity) {
		answer->list.entity = mpd_tag_entity_begin(pair);
	} else if (!mpd_tag_entity_feed(answer->list.entity, pair)) {
		answer->list.list = g_list_prepend(answer->list.list, answer->list.entity);
		answer->list.entity = mpd_tag_entity_begin(pair);
	}

	return TRUE;
}

gboolean parse_pair_idle(union mpd_cmd_answer *answer, const struct mpd_pair *pair)
{
	if (strcmp(pair->name, "changed")) {
		return FALSE;
	}
	if (!strcmp("database", pair->value)) {
		answer->idle |= MPD_CHANGED_DB;
	} else if (!strcmp("update", pair->value)) {
		answer->idle |= MPD_CHANGED_UPDATE;
	} else if (!strcmp("stored_playlist", pair->value)) {
		answer->idle |= MPD_CHANGED_STORED_PL;
	} else if (!strcmp("playlist", pair->value)) {
		answer->idle |= MPD_CHANGED_PL;
	} else if (!strcmp("player", pair->value)) {
		answer->idle |= MPD_CHANGED_PLAYER;
	} else if (!strcmp("mixer", pair->value)) {
		answer->idle |= MPD_CHANGED_MIXER;
	} else if (!strcmp("output", pair->value)) {
		answer->idle |= MPD_CHANGED_OUTPUT;
	} else if (!strcmp("options", pair->value)) {
		answer->idle |= MPD_CHANGED_OPTIONS;
	} else if (!strcmp("sticker", pair->value)) {
		answer->idle |= MPD_CHANGED_STICKER;
	} else if (!strcmp("subscription", pair->value)) {
		answer->idle |= MPD_CHANGED_SUBSCR;
	} else if (!strcmp("message", pair->value)) {
		answer->idle |= MPD_CHANGED_MESSAGE;
	} else {
		return FALSE;
	}
	MSG_DEBUG("changed: %s", pair->value);

	return TRUE;
}

#define MPD_GREETING "OK MPD"

gboolean mpd_recv(struct mpd_source *source)
{
	char *line;
	enum mpd_parser_result result;
	struct mpd_cmd *cmd;
	struct mpd_pair pair;
	gboolean end = FALSE;
	gboolean success = FALSE;
	struct mpd_cmd_cb *cur;

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

		if (cmd->type == MPD_CMD_NONE && !strncmp(line, MPD_GREETING, strlen(MPD_GREETING))) {
			success = TRUE;
			break;
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
			if (cmd->parse_pair) {
				cmd->parse_pair(&cmd->answer, &pair);
			}
			break;
		}
	}

	if (success) {
		if (cmd->process) {
			cmd->process(&cmd->answer);
		}
		for (cur = source->cbs[cmd->type]; cur; cur = cur->next) {
			cur->cb(cmd->args, &cmd->answer, cur->data);
		}
	}
	mpd_cmd_free(cmd);
	g_queue_pop_head(&source->pending);

	if (g_queue_is_empty(&source->pending)) {
		mpd_send((GSource *) source, MPD_CMD_IDLE, NULL);
	}

	return TRUE;
}

gboolean mpd_cmd_send(GSource *source, struct mpd_cmd *cmd, ...)
{
	va_list args;
	gboolean retval;

	va_start(args, cmd);
	retval = mpd_cmd_send_v(source, cmd, args);
	va_end(args);

	return retval;
}

gboolean mpd_cmd_send_v(GSource *source, struct mpd_cmd *cmd, va_list args)
{
	struct mpd_source *mpdsource = (struct mpd_source *) source;
	va_list args2;
	struct mpd_cmd *pending;
	gboolean retval;
	const char *arg;
	const char *cmd_str;

	cmd_str = mpd_cmd_to_str(cmd->type);

	pending = g_queue_peek_tail(&mpdsource->pending);

	if (pending && pending->type == MPD_CMD_IDLE) {
		MSG_DEBUG("stop idling");
		mpd_async_send_command(mpdsource->async, "noidle", NULL);
	}

	MSG_INFO("sending MPD command %s", cmd_str);

	va_copy(args2, args);
	retval = mpd_async_send_command_v(mpdsource->async, cmd_str, args);
	mpd_async_io(mpdsource->async, MPD_ASYNC_EVENT_WRITE);

	if (!retval) {
		va_end(args2);
		return FALSE;
	}

	while ((arg = va_arg(args2, const char *)) != NULL) {
		cmd->args = g_list_prepend(cmd->args, g_strdup(arg));
	}
	cmd->args = g_list_reverse(cmd->args);
	g_queue_push_tail(&mpdsource->pending, cmd);
	va_end(args2);

	return TRUE;
}

gboolean mpd_send(GSource *source, enum mpd_cmd_type type, ...)
{
	va_list args;
	const char *cmd_str;
	struct mpd_cmd *cmd;
	gboolean retval;

	if (!source) {
		MSG_ERROR("mpd_send(): invalid source");
		return FALSE;
	}

	cmd_str = mpd_cmd_to_str(type);
	if (!cmd_str) {
		MSG_WARNING("invalid command");
		return FALSE;
	}

	cmd = mpd_cmd_new(type);
	va_start(args, type);
	retval = mpd_cmd_send_v(source, cmd, args);
	va_end(args);

	return retval;
}

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
	struct mpd_cmd *greeting;
	int i;

	source = g_source_new(&mpdsourcefuncs, sizeof(struct mpd_source));
	mpdsource = (struct mpd_source *) source;
	mpdsource->fd = g_source_add_unix_fd(source, fd, G_IO_IN | G_IO_OUT | G_IO_HUP | G_IO_ERR);
	mpdsource->async = mpd_async_new(fd);
	g_source_set_priority(source, G_PRIORITY_DEFAULT_IDLE);

	mpdsource->parser = mpd_parser_new();
	g_queue_init(&mpdsource->pending);

	greeting = mpd_cmd_new(MPD_CMD_NONE);
	g_queue_push_tail(&mpdsource->pending, greeting);

	for (i = 0; i < MPD_CMD_COUNT; i++) {
		mpdsource->cbs[i] = NULL;
	}

	return source;
}

void mpd_source_close(GSource *source)
{
	struct mpd_source *mpdsource = (struct mpd_source *) source;
	struct mpd_cmd *cmd;
	int i;
	struct mpd_cmd_cb *cur, *next;

	for (i = 0; i < MPD_CMD_COUNT; i++) {
		for (cur = mpdsource->cbs[i]; cur; cur = next) {
			next = cur->next;
			g_free(cur);
		}
	}

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

void mpd_source_register(GSource *source, enum mpd_cmd_type cmd, CMDCallback cb, void *data)
{
	struct mpd_source *mpdsource = (struct mpd_source *) source;

	mpdsource->cbs[cmd] = mpd_cmd_cb_append(mpdsource->cbs[cmd], cb, data);
}

struct mpd_cmd_cb *mpd_cmd_cb_append(struct mpd_cmd_cb *list, CMDCallback cb, void *data)
{
	struct mpd_cmd_cb *new;
	struct mpd_cmd_cb *cur;

	new = g_malloc(sizeof(struct mpd_cmd_cb));
	new->next = NULL;
	new->cb = cb;
	new->data = data;

	if (!list) {
		return new;
	}

	for (cur = list; cur->next; cur = cur->next);
	cur->next = new;

	return list;
}
