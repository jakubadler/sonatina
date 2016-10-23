#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <gtk/gtk.h>

#include "core.h"
#include "client.h"

enum pl_columns {
	PL_ID,
	PL_POS,
	PL_WEIGHT,
	PL_COUNT
};

struct pl_tab {
	struct sonatina_tab tab;
	GtkBuilder *ui;
	size_t n_columns; /** Number of user-defined columns */
	gchar **columns; /** Format of user-defined columns; NULL-terminated array of length n_columns */
	GtkListStore *store; /** Contains internal coulumns and user-defined
			       columns. Number of coulumns is PL_COUNT + n_columns */
};

gboolean pl_tab_init(struct sonatina_tab *tab);
void pl_tab_set_source(struct sonatina_tab *tab, GSource *source);
void pl_tab_destroy(struct sonatina_tab *tab);

/**
  @brief Set one song on the playlist to be displayed as currently playing.
  @param pl Playlist.
  @param pos_req Position of the song on the playlist.
  */
void pl_set_active(struct pl_tab *pl, int pos_req);
void pl_update(struct pl_tab *pl, const struct mpd_song *song);

void playlist_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);

void pl_process_song(union mpd_cmd_answer *answer, void *data);
void pl_process_pl(union mpd_cmd_answer *answer, void *data);

#endif
