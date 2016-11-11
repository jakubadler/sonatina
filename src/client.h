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
	MPD_CMD_SEEKCUR,
	MPD_CMD_SETVOL,
	MPD_CMD_PLINFO,
	MPD_CMD_CLOSE,
	MPD_CMD_LIST,
	MPD_CMD_LSINFO,
	MPD_CMD_FIND,
	MPD_CMD_ADD,
	MPD_CMD_FINDADD,
	MPD_CMD_CLEAR,
	MPD_CMD_UPDATE,
	MPD_CMD_COUNT
};

union mpd_cmd_answer {
	void *ptr;
	struct mpd_song *song; /* MPD_CMD_CURRENTSONG */
	struct mpd_status *status; /* MPD_CMD_STATUS */
	struct mpd_stats *stats; /* MPD_CMD_STATS */
	struct {
		struct mpd_song *song;
		GList *list;
	} plinfo; /* MPD_CMD_PLINFO */
	struct {
		struct mpd_entity *entity;
		GList *list;
	} lsinfo; /* MPD_CMD_LSINFO */
	GList *list; /* MPD_CMD_LIST */
	int idle; /* MPD_CMD_IDLE */
	gboolean ok;
};

typedef void (*CMDCallback)(GList *args, union mpd_cmd_answer *, void *);

struct mpd_cmd_cb {
	CMDCallback cb;
	void *data;
	struct mpd_cmd_cb *next;
};

/**
  @brief GSource to be used for asynchronous communication with MPD server.
  */
struct mpd_source {
	GSource source;
	gpointer fd;
	struct mpd_async *async;
	struct mpd_parser *parser;
	GQueue pending;
	struct mpd_cmd_cb *cbs[MPD_CMD_COUNT];
};

/**
  @brief Structure representing a MPD command.
  */
struct mpd_cmd {
	enum mpd_cmd_type type;
	GList *args;
	union mpd_cmd_answer answer;
	gboolean (*parse_pair)(union mpd_cmd_answer *answer,
			const struct mpd_pair *pair); /** Function to parse single pair. Called each time a pair is received */
	void (*process)(union mpd_cmd_answer *answer); /** Function to process received answer. Called after whole answer is received */
	void (*free_answer)(union mpd_cmd_answer *answer);
};

#define MPD_CHANGED_DB		0x001
#define MPD_CHANGED_UPDATE	0x002
#define MPD_CHANGED_STORED_PL	0x004
#define MPD_CHANGED_PL		0x008
#define MPD_CHANGED_PLAYER	0x00f
#define MPD_CHANGED_MIXER	0x010
#define MPD_CHANGED_OUTPUT	0x020
#define MPD_CHANGED_OPTIONS	0x040
#define MPD_CHANGED_STICKER	0x080
#define MPD_CHANGED_SUBSCR	0x0f0
#define MPD_CHANGED_MESSAGE	0x100

const char *mpd_cmd_to_str(enum mpd_cmd_type cmd);
/**
  @brief Create new mpd command.
  @param type Type of the new command.
  @param ... NULL terminated list of command arguments.
  @returns Newly allocated command that should be freed with @a mpd_cmd_free().
  */
struct mpd_cmd *mpd_cmd_new(enum mpd_cmd_type type);

/**
  @brief Free data allocated by MPD command.
  @param cmd MPD command
  */
void mpd_cmd_free(struct mpd_cmd *cmd);

/**
  @brief Do actions according to received answer to MPD command. This function
  should be called only after a complete answer is received.
  @param cmd MPD command
  */
void mpd_cmd_process_answer(struct mpd_cmd *cmd);

gboolean parse_pair_status(union mpd_cmd_answer *answer, const struct mpd_pair *pair);
gboolean parse_pair_song(union mpd_cmd_answer *answer, const struct mpd_pair *pair);
gboolean parse_pair_plsong(union mpd_cmd_answer *answer, const struct mpd_pair *pair);
gboolean parse_pair_lsinfo(union mpd_cmd_answer *answer, const struct mpd_pair *pair);
gboolean parse_pair_list(union mpd_cmd_answer *answer, const struct mpd_pair *pair);
gboolean parse_pair_idle(union mpd_cmd_answer *answer, const struct mpd_pair *pair);

void cmd_process_song(union mpd_cmd_answer *answer);
void cmd_process_status(union mpd_cmd_answer *answer);
void cmd_process_idle(union mpd_cmd_answer *answer);
void cmd_process_plinfo(union mpd_cmd_answer *answer);
void cmd_process_lsinfo(union mpd_cmd_answer *answer);
void cmd_process_list(union mpd_cmd_answer *answer);

/**
  @brief Function to create TCP connection to the server.
  @param host Host name
  @param port TCP port
  @returns File descriptor of TCP connection to server.
  */
int client_connect(const char *host, int port);

/**
  @brief Create a new MPD source.
  @param fd File descriptor of a connetcion to MPD server.
  @returns Newly created MPD source that should be destroyed with @a
  mpd_source_close();
  */
GSource *mpd_source_new(int fd);

/**
  @brief Close connection associated with this MPD source and free all internal
  data.
  @param source MPD source
  */
void mpd_source_close(GSource *source);


/**
  @brief Receive one MPD response (i.e. ending with OK or ACK line) from
  mpd source and do according actions.
  @param source MPD source
  @returns TRUE if a response was received successfully, FALSE otherwise.
  */
gboolean mpd_recv(struct mpd_source *source);


extern GSourceFuncs mpdsourcefuncs;

/**
  @brief Part of GSource implementation.
  */
gboolean mpd_prepare(GSource *source, gint *timeout);

/**
  @brief Part of GSource implementation.
  */
gboolean mpd_check(GSource *source);

/**
  @brief Part of GSource implementation.
  */
gboolean mpd_dispatch(GSource *source, GSourceFunc callback, gpointer data);

/**
  @brief Send command to MPD server.
  @param source MPD source connected to a MPD server.
  @param cmd Command to send.
  @param ... List of command arguments terminated with NULL.
  @returns TRUE when command was sent, FALSE otherwise.
  */
gboolean mpd_cmd_send(GSource *source, struct mpd_cmd *cmd, ...);

/**
  va_list version of @a mpd_cmd_send();
  @param source MPD source connected to a MPD server.
  @param cmd Command to send.
  @param ... List of command arguments terminated with NULL.
  @returns TRUE when command was sent, FALSE otherwise.
  */
gboolean mpd_cmd_send_v(GSource *source, struct mpd_cmd *cmd, va_list args);

/**
  Create and send MPD command with arguments.
  @param source MPD source connected to a MPD server.
  @param type Command type.
  @param ... NULL terminated list of arguments.
  @returns TRUE when command was sent, FALSE otherwise.
  */
gboolean mpd_send(GSource *source, enum mpd_cmd_type type, ...);

/**
  @brief Register a callback that will be called when an answer to a comand is
  received in addition to the command's own process function.
  @param source MPD source
  @param cmd Command type
  @param cb Callback function
  */
void mpd_source_register(GSource *source, enum mpd_cmd_type cmd, CMDCallback cb, void *data);

struct mpd_cmd_cb *mpd_cmd_cb_append(struct mpd_cmd_cb *list, CMDCallback cb, void *data);

#endif
