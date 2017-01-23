#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <glib.h>

#include "settings.h"
#include "core.h"
#include "util.h"
#include "gettext.h"

gchar *rcdir;
gchar *rcfile;

GKeyFile *rc;

struct settings_entry settings[] = {
	{ "main", "active_profile", SETTINGS_STRING, NULL, NULL, NULL },
	{ "main", "title", SETTINGS_STRING, __("Song line 1"), NULL, NULL },
	{ "main", "subtitle", SETTINGS_STRING, __("Song line 2"), NULL, NULL },
	{ "playlist", "format", SETTINGS_STRING, __("Playlist entry"), NULL, NULL },
	{ "library", "format", SETTINGS_STRING, __("Library entry"), NULL, NULL },
	{ "library", "icon_size", SETTINGS_NUM, __("Icon size"), NULL, NULL },
	{ NULL, NULL, SETTINGS_UNKNOWN, NULL, NULL, NULL }
};

void sonatina_settings_default(GKeyFile *rc)
{
	if (!g_key_file_get_string(rc, "main", "title", NULL))
		g_key_file_set_string(rc, "main", "title", DEFAULT_MAIN_TITLE);
	if (!g_key_file_get_string(rc, "main", "subtitle", NULL))
		g_key_file_set_string(rc, "main", "subtitle", DEFAULT_MAIN_SUBTITLE);
	if (!g_key_file_get_string(rc, "playlist", "format", NULL))
		g_key_file_set_string(rc, "playlist", "format", DEFAULT_PLAYLIST_FORMAT);
	if (!g_key_file_get_string(rc, "library", "format", NULL))
		g_key_file_set_string(rc, "library", "format", DEFAULT_LIBRARY_FORMAT);
	if (!g_key_file_get_integer(rc, "library", "icon_size", NULL))
		g_key_file_set_integer(rc, "library", "icon_size", DEFAULT_LIBRARY_ICON_SIZE);
}

gboolean sonatina_settings_load()
{
	size_t i;

	if (!rcdir) {
		rcdir = g_build_filename(g_get_user_config_dir(), PACKAGE, NULL);
	}

	if (!rcfile) {
		rcfile = g_build_filename(rcdir, "sonatinarc", NULL);
	}

	for (i = 0; settings[i].section; i++) {
		if (settings[i].label) {
			settings[i].label = gettext(settings[i].label);
		}
		if (settings[i].tooltip) {
			settings[i].tooltip = gettext(settings[i].tooltip);
		}
	}
	
	if (!rc) {
		rc = g_key_file_new();
	}

	g_key_file_load_from_file(rc, rcfile,
	                                 G_KEY_FILE_KEEP_COMMENTS |
	                                 G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

	sonatina_settings_default(rc);

	return TRUE;
}

void sonatina_settings_save()
{
	g_key_file_save_to_file(rc, rcfile, NULL);
	g_key_file_free(rc);
}

enum settings_type sonatina_settings_get(const char *section, const char *name, union settings_value *value)
{
	const struct settings_entry *entry;
	union settings_value val;
	GError *err = NULL;

	g_assert(section != NULL);
	g_assert(name != NULL);

	entry = settings_lookup(section, name, SETTINGS_UNKNOWN);

	if (!entry) {
		MSG_ERROR("Undefined settings entry %s/%s", section, name);
		return SETTINGS_UNKNOWN;
	}

	switch (entry->type) {
	case SETTINGS_NUM:
		val.num = g_key_file_get_integer(rc, section, name, &err);
		break;
	case SETTINGS_BOOL:
		val.boolean = g_key_file_get_boolean(rc, section, name, &err);
		break;
	case SETTINGS_STRING:
		val.string = g_key_file_get_string(rc, section, name, &err);
		break;
	default:
		MSG_ERROR("Invalid settings entry type: %d", entry->type);
		return SETTINGS_UNKNOWN;
		break;
	}

	if (err) {
		MSG_ERROR("Couldn't retrieve settings entry %s/%s: %s", entry->section, entry->name, err->message);
		g_error_free(err);
		return SETTINGS_UNKNOWN;
	}

	*value = val;

	return entry->type;
}

gboolean sonatina_settings_set(const char *section, const char *name, union settings_value value)
{
	const struct settings_entry *entry;

	g_assert(section != NULL);
	g_assert(name != NULL);

	entry = settings_lookup(section, name, SETTINGS_UNKNOWN);

	if (!entry) {
		MSG_ERROR("Undefined settings entry %s/%s", section, name);
		return FALSE;
	}

	switch (entry->type) {
	case SETTINGS_NUM:
		MSG_DEBUG("settings %s/%s = %d", section, name, value.num);
		g_key_file_set_integer(rc, section, name, value.num);
		break;
	case SETTINGS_BOOL:
		MSG_DEBUG("settings %s/%s = %d", section, name, value.boolean);
		g_key_file_set_boolean(rc, section, name, value.boolean);
		break;
	case SETTINGS_STRING:
		g_assert(value.string != NULL);
		MSG_DEBUG("settings %s/%s = %s", section, name, value.string);
		g_key_file_set_string(rc, section, name, value.string);
		break;
	default:
		MSG_ERROR("Invalid settings entry type: %d", entry->type);
		return FALSE;
		break;
	}

	return TRUE;
}

const struct settings_entry *settings_lookup(const char *section, const char *name, enum settings_type type)
{
	size_t i;

	for (i = 0; settings[i].section; i++) {
		if (!g_strcmp0(settings[i].section, section) &&
		    !g_strcmp0(settings[i].name, name) &&
		    (settings[i].type == type || type == SETTINGS_UNKNOWN)) {
			return &settings[i];
		}
	}

	return NULL;
}

gint sonatina_settings_get_num(const char *section, const char *name)
{
	enum settings_type type;
	union settings_value val;

	type = sonatina_settings_get(section, name, &val);

	if (type == SETTINGS_NUM) {
		return val.num;
	}

	if (type == SETTINGS_STRING) {
		g_free(val.string);
	}

	return 0;
}

gboolean sonatina_settings_get_bool(const char *section, const char *name)
{
	enum settings_type type;
	union settings_value val;

	type = sonatina_settings_get(section, name, &val);

	if (type == SETTINGS_BOOL) {
		return val.boolean;
	}

	if (type == SETTINGS_STRING) {
		g_free(val.string);
	}

	return FALSE;
}

gchar *sonatina_settings_get_string(const char *section, const char *name)
{
	enum settings_type type;
	union settings_value val;

	type = sonatina_settings_get(section, name, &val);

	if (type == SETTINGS_STRING) {
		return val.string;
	}

	return NULL;
}

