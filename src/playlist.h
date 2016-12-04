#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <gtk/gtk.h>

#include "core.h"
#include "client.h"
#include "settings.h"

enum pl_columns {
	PL_ID,
	PL_POS,
	PL_WEIGHT,
	PL_COUNT
};

/**
  Structure derivated from struct sonatina_tab.
  */
struct pl_tab {
	struct sonatina_tab tab;
	GtkBuilder *ui; /** GTK builder containing UI definitons of the playlist
			 tab */
	GSource *mpdsource; /** Connected MPD source or NULL */
	size_t n_columns; /** Number of user-defined columns */
	gchar **columns; /** Format of user-defined columns; NULL-terminated array of length n_columns */
	GtkListStore *store; /** Contains internal coulumns and user-defined
			       columns. Number of coulumns is PL_COUNT + n_columns */
};

/**
  @brief Initialize playlist tab.
  @param tab Tab to initialize.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean pl_tab_init(struct sonatina_tab *tab);

void pl_tab_set_format(struct pl_tab *tab, const char *format);

/**
  @brief Set MPD source of a playlist tab.
  @param tab Playlist tab
  @param source MPD source or NULL when not connected.
  */
void pl_tab_set_source(struct sonatina_tab *tab, GSource *source);

/**
  @brief Free memory allocated by playlist tab.
  @param tab Playlist tab.
  */
void pl_tab_destroy(struct sonatina_tab *tab);

/**
  @brief Set one song on the playlist to be displayed as currently playing.
  @param pl Playlist tab.
  @param pos_req Position of the song on the playlist.
  */
void pl_set_active(struct pl_tab *pl, int pos_req);

/**
  @brief Update single song on a playlist. Changes song on given position or
  adds a new one when the position doesn't exist.
  @param pl Playlist tab.
  @param song MPD song.
  */
void pl_update(struct pl_tab *pl, const struct mpd_song *song);

/**
  @brief GTK callback called when a playlist is clicked. Start playing activated
  song.
  */
void playlist_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);

void pl_process_song(GList *args, union mpd_cmd_answer *answer, void *data);
void pl_process_pl(GList *args, union mpd_cmd_answer *answer, void *data);

void pl_selection_changed(GtkTreeSelection *selection, gpointer data);

void playlist_remove_action(GSimpleAction *action, GVariant *param, gpointer data);
void playlist_clear_action(GSimpleAction *action, GVariant *param, gpointer data);

#endif
