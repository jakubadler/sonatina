#ifndef SETTINGS_H
#define SETTINGS_H

#include <gtk/gtk.h>
#include <mpd/client.h>

#define SETTINGS_FILE ".config/sonatina/settings.rc"

struct sonatina_settings {
	int art_size;
};

gboolean sonatina_settings_load(struct sonatina_settings *settings, const char *path);
void sonatina_settings_default(struct sonatina_settings *s);
gboolean sonatina_settings_apply();

#endif
