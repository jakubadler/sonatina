#ifndef LIBRARY_H
#define LIBRARY_H

#include "core.h"

enum listing_type {
	LIBRARY_FS,
	LIBRARY_GENRE,
	LIBRARY_ARTIST,
	LIBRARY_ALBUM,
	LIBRARY_COUNT
};

struct library_dir_entry {
	struct library_dir_entry *prev;
	struct library_dir_entry *next;
	char *name;
};

struct library_path {
	enum listing_type type;
	struct library_path *parent;
	struct library_path *next;
	struct library_dir_entry *entries;
};

struct library_tab {
	struct sonatina_tab tab;
	struct library_path path;
	GtkBuilder *ui;
};

gboolean library_tab_init(struct sonatina_tab *tab);
void library_tab_set_source(struct sonatina_tab *tab, GSource *source);
void library_tab_destroy(struct sonatina_tab *tab);

#endif
