#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>
#include <gtk/gtk.h>

#define DEFAULT_MAIN_TITLE "%T"
#define DEFAULT_MAIN_SUBTITLE "%A - %B"
#define DEFAULT_PLAYLIST_FORMAT "%N|%T|%A"
#define DEFAULT_LIBRARY_FORMAT "%N %T"
#define DEFAULT_LIBRARY_ICON_SIZE (GTK_ICON_SIZE_BUTTON)

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
	void (*changed_cb)(union settings_value, void *);

};

/**
  @brief Load default settings.
  @param rc Key file
  */
void sonatina_settings_default(GKeyFile *rc);

/**
  @brief Load settings from file.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_settings_load();

/**
  @brief Save runtime settings to files.
  */
void sonatina_settings_save();

/**
  @brief Get value of a settings entry. An entry is specified by its name and a
  category and has a type.
  @param section Section name.
  @param type Entry name.
  @param value Address where to store value.
  @returns Type of the retrieved value or SETTINGS_UNKNOW when no value was set.
  */
enum settings_type sonatina_settings_get(const char *section, const char *name, union settings_value *value);

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

/**
  @brief Convenience wrapper over @a sonatina_settings_get() to retrive numeric
  type settings entry value.
  @param section Section name.
  @param name Entry name.
  @returns Value of the specified entry or 0 if the entry doesn't exist or is of
  another type.
  */
gint sonatina_settings_get_num(const char *section, const char *name);

/**
  @brief Convenience wrapper over @a sonatina_settings_get() to retrive boolean
  type settings entry value.
  @param section Section name.
  @param name Entry name.
  @returns Value of the specified entry or FALSE if the entry doesn't exist or is of
  another type.
  */
gboolean sonatina_settings_get_bool(const char *section, const char *name);

/**
  @brief Convenience wrapper over @a sonatina_settings_get() to retrive string
  type settings entry value.
  @param section Section name.
  @param name Entry name.
  @returns Value of the specified entry or NULL if the entry doesn't exist or is of
  another type. If non-NULL is returned, it should be freed with g_free().
  */
gchar *sonatina_settings_get_string(const char *section, const char *name);

#endif
