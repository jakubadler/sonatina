#ifndef CORE_H
#define CORE_H

#include <glib.h>
#include <gtk/gtk.h>

#include "mpd/client.h"

/**
  @brief Structure holding GtkListStore for a playlist and formats for its columns.
  */
struct playlist {
	size_t n_columns; /** Number of user-defined columns */
	gchar **columns; /** Format of user-defined columns; NULL-terminated array of length n_columns */
	GtkListStore *store; /** Contains internal coulumns and user-defined
			       columns. Number of coulumns is PL_COUNT + n_columns */
};

enum pl_columns {
	PL_ID,
	PL_POS,
	PL_WEIGHT,
	PL_COUNT
};

/**
  @brief Structure holding data of a running sonatina instance.
  */
struct sonatina_instance {
	GSource *mpdsource; /** NULL when not connected */

	GtkBuilder *gui;
	GKeyFile *rc;
	GList *profiles; /** List of GKeyFile objects representing available profiles */

	struct playlist pl;
	int cur;

	int elapsed;
	int total;
	GTimer *counter;
};

extern struct sonatina_instance sonatina;

void sonatina_init();
void sonatina_destroy();

/**
  @brief Connect sonatina instance to a MPD server.
  @param host Host name.
  @param port TCP port.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_connect(const char *host, int port);

/**
  @brief Disconnect sonatina instance from MPD server.
  */
void sonatina_disconnect();

/**
  @brief Change connection profile on sonatina instance.
  @param name Name of the profile.
  @returns TRUE when profile was successfully changed.
  */
gboolean sonatina_change_profile(const char *name);

/**
  @brief Update current song on a sonatina instance.
  @param song MPD song struct.
  */
void sonatina_update_song(const struct mpd_song *song);

/**
  @brief Update status on a sonatina instance.
  @param status MPD status struct.
  */
void sonatina_update_status(const struct mpd_status *status);

/**
  @brief Update single song on playlist on a sonatina instance.
  @param song MPD song struct.
  */
void sonatina_update_pl(const struct mpd_song *song);

/**
  @brief Clear playlist on a sonatina instance.
  */
void sonatina_clear_pl();

/**
  @brief Initialize playlist structure with given format.
  @param pl Pointer to playlist structure.
  @param format Format string specifying format of each column separated by '|'.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean pl_init(struct playlist *pl, const char *format);

/**
  @brief Update playlist with single song.
  @param pl playlist structure
  @param song MPD song structure
  */
void pl_update(struct playlist *pl, const struct mpd_song *song);

/**
  @brief Set one song on the playlist to be displayed as currently playing.
  @param pl Playlist.
  @param pos_req Position of the song on the playlist.
  */
void pl_set_active(struct playlist *pl, int pos_req);

/**
  @brief Free data allocated by playlist.
  @param pl playlist structure
  */
void pl_free(struct playlist *pl);

/**
  @brief Function called every second to check elapsed time and update progressbar.
  @param data Unused user data.
  @returns TRUE
  */
gboolean counter_cb(gpointer data);

/**
  @brief Convenience function to send mpd command 'play' with integer argument.
  @param pos Position attribute of the song to be played.
  */
void sonatina_play(int pos);

/**
  @brief Convenience function to send mpd command 'seekcur' with integer argument.
  @param time Time where to seek to.
  */
void sonatina_seek(int time);

/**
  @brief Convenience function to send mpd command 'setvol' with integer argument.
  @param vol Volume as a value in range between 0 and 1.
  */
void sonatina_setvol(double vol);

#endif
