#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>

#define DEFAULT_MAIN_TITLE "%T"
#define DEFAULT_MAIN_SUBTITLE "%A - %B"
#define DEFAULT_PLAYLIST_FORMAT "%N|%T|%A"
#define DEFAULT_LIBRARY_FORMAT "%N %T"

enum settings_type {
	SETTINGS_UNKNOWN,
	SETTINGS_NUM,
	SETTINGS_BOOL,
	SETTINGS_STRING
};

union settings_value {
	gint num;
	gboolean boolean;
	gchar *string;
};

struct settings_entry {
	const char *section;
	const char *name;
	enum settings_type type;
	const char *label;
	const char *tooltip;

};

/**
  @brief Load default settings.
  @param rc Key file
  */
void sonatina_settings_default(GKeyFile *rc);

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

/**
  @brief Get value of a settings entry. An entry is specified by its name and a
  category and has a type.
  @param section Section name.
  @param type Entry name.
  @param value Address where to store value.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_settings_get(const char *section, const char *name, union settings_value *value);

/**
  @brief Set value of a settings entry.
  @param section Section name.
  @param type Entry name.
  @param value Value to set.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_settings_set(const char *section, const char *name, union settings_value value);

/**
  @brief Search an entry in settings database. Requested entry is specified by
  its section, name and optionally its type. If no such entry is found, NULL is
  returned.
  @param section Section name.
  @param name Entry name.
  @param type Entry type or SETTINGS_UNKNOWN if type doesn't matter.
  @returns Settings entry found or NULL.
  */
const struct settings_entry *settings_lookup(const char *section, const char *name, enum settings_type type);

#endif
