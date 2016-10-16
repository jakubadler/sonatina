#ifndef CORE_H
#define CORE_H

#include <glib.h>

#include "settings.h"

struct playlist {
	size_t n_columns;
	gchar **columns;
	GtkListStore *store;
};

struct sonatina_instance {
	GSource *mpdsource;

	GtkBuilder *gui;
	GKeyFile *rc;
	GList *profiles;

	struct playlist pl;

	GtkListStore *plstore;
};

extern struct sonatina_instance sonatina;

void sonatina_init();
gboolean sonatina_connect(const char *host, int port);
void sonatina_disconnect();

gboolean sonatina_change_profile(const char *name);

void sonatina_update_song(const struct mpd_song *song);
void sonatina_update_status(const struct mpd_status *status);
void sonatina_update_pl(const struct mpd_song *song);
void sonatina_clear_pl();

gboolean pl_init(struct playlist *pl, const char *format);
void pl_update(struct playlist *pl, const struct mpd_song *song);
void pl_free(struct playlist *pl);

#endif
