#ifndef SETTINGS_H
#define SETTINGS_H

#include <gtk/gtk.h>
#include <mpd/client.h>

#define SETTINGS_FILE ".config/sonatina/settings.rc"

#define DEFAULT_PLAYLIST_FORMAT "%N|%T|%A"

gboolean sonatina_settings_load();
void sonatina_add_profile(const char *name, const char *host, int port);
GKeyFile *sonatina_get_profile(const char *name);

#endif
