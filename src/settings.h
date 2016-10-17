#ifndef SETTINGS_H
#define SETTINGS_H

#include <gtk/gtk.h>
#include <mpd/client.h>

#define SETTINGS_FILE ".config/sonatina/settings.rc"

#define DEFAULT_PLAYLIST_FORMAT "%N|%T|%A"

/**
  @brief Load settings from file to sonatina.rc.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_settings_load();

/**
  @brief Save runtime settings to files.
  */
void sonatina_settings_save();

/**
  @brief Create new connection profile.
  @param name Name of the new profile.
  @param host Hostname.
  @param port TCP port.
  */
void sonatina_add_profile(const char *name, const char *host, int port);

/**
  @brief Get connection profile by name.
  @param name Name of the profile.
  @returns Key file representation of a connection profile.
  */
GKeyFile *sonatina_get_profile(const char *name);

#endif
