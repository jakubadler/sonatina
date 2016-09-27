#ifndef CORE_H
#define CORE_H

#include "settings.h"

struct sonatina_instance {
	GSource *mpdsource;

	struct mpd_song *current_song;

	GtkBuilder *gui;
	GKeyFile *rc;
};

extern struct sonatina_instance sonatina;

void sonatina_init();
void sonatina_connect(const char *host, int port);

void sonatina_change_art(const char *path);

#endif
