#ifndef LIBRARY_H
#define LIBRARY_H

#include "client.h"
#include "core.h"

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

void library_list_cb(GList *args, union mpd_cmd_answer *answer, void *data);
void library_lsinfo_cb(GList *args, union mpd_cmd_answer *answer, void *data);

void library_load(struct library_tab *tab);
struct library_path *library_path_open(struct library_path *parent, const char *name);
void library_path_close(struct library_path *path);

struct library_path *library_path_root(enum listing_type listing);
struct library_path *library_path_open(struct library_path *path, const char *name);
void library_path_free(struct library_path *path);

void library_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, struct library_tab *tab);

void library_model_append_entity(GtkListStore *model, const struct mpd_entity *entity);

/**
  Get path string for given path.
  */
gchar *library_path_get_uri(const struct library_path *root, const struct library_path *path);

#endif
