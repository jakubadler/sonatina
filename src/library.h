#ifndef LIBRARY_H
#define LIBRARY_H

#include "client.h"
#include "core.h"
#include "pathbar.h"

enum listing_type {
	LIBRARY_FS,
	LIBRARY_GENRE,
	LIBRARY_ARTIST,
	LIBRARY_ALBUM,
	LIBRARY_SONG,
	LIBRARY_COUNT
};


struct library_path {
	enum listing_type type;
	struct library_path *parent;
	struct library_path *next;
	
	char *name;
	int selected;
};

struct library_tab {
	struct sonatina_tab tab;
	GtkBuilder *ui;
	GSource *mpdsource;
	GtkListStore *store;
	SonatinaPathBar *pathbar;
	struct library_path *root;
	struct library_path *path;
};

enum lib_columns {
	LIB_COL_NAME,
	LIB_COL_COUNT
};

gboolean library_tab_init(struct sonatina_tab *tab);
void library_tw_set_columns(GtkTreeView *tw);
void library_tab_set_source(struct sonatina_tab *tab, GSource *source);
void library_tab_destroy(struct sonatina_tab *tab);

void library_tab_open_dir(struct library_tab *tab, const char *name);
void library_tab_open_pos(struct library_tab *tab, int pos);

void library_list_cb(GList *args, union mpd_cmd_answer *answer, void *data);
void library_lsinfo_cb(GList *args, union mpd_cmd_answer *answer, void *data);

void library_pathbar_changed(SonatinaPathBar *pathbar, gint selected, gpointer data);

void library_set_listing(struct library_tab *tab, enum listing_type listing);
void library_load(struct library_tab *tab);
void library_path_close(struct library_path *path);

struct library_path *library_path_root(enum listing_type listing);
struct library_path *library_path_open(struct library_path *path, const char *name);
void library_path_free(struct library_path *path);
void library_path_free_all(struct library_path *path);

void library_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, struct library_tab *tab);

void library_model_append_entity(GtkListStore *model, const struct mpd_entity *entity);
void library_model_append(GtkListStore *model, enum listing_type type, const char *name);

/**
  Get path string for given path.
  */
gchar *library_path_get_uri(const struct library_path *root, const struct library_path *path);

#endif
