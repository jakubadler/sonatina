#ifndef CORE_H
#define CORE_H

#include <glib.h>

#include "settings.h"

struct sonatina_instance {
	GSource *mpdsource;

	struct mpd_song *current_song;

	GtkBuilder *gui;
	GKeyFile *rc;
	GList *profiles;
};

extern struct sonatina_instance sonatina;

void sonatina_init();
gboolean sonatina_connect(const char *host, int port);
void sonatina_disconnect();

gboolean sonatina_change_profile(const char *name);

#endif
