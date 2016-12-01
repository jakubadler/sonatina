#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <glib.h>

#include "settings.h"
#include "core.h"
#include "util.h"
#include "gui.h"
#include "gettext.h"

gchar *rcdir;
gchar *rcfile;

GKeyFile *profiles;

struct settings_entry settings[] = {
	{ "main", "title", SETTINGS_STRING, __("Song line 1"), NULL },
	{ "main", "subtitle", SETTINGS_STRING, __("Song line 2"), NULL },
	{ "playlist", "format", SETTINGS_STRING, __("Playlist entry"), NULL },
	{ "library", "format", SETTINGS_STRING, __("Library entry"), NULL },
	{ NULL, NULL, SETTINGS_UNKNOWN, NULL, NULL }
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
	
	if (!sonatina.rc) {
		sonatina.rc = g_key_file_new();
	}

	g_key_file_load_from_file(sonatina.rc, rcfile,
	                                 G_KEY_FILE_KEEP_COMMENTS |
	                                 G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

	return TRUE;
}

void sonatina_settings_save()
{
	g_key_file_save_to_file(sonatina.rc, rcfile, NULL);
}

gboolean sonatina_settings_get(const char *section, const char *name, union settings_value *value)
{
	const struct settings_entry *entry;
	union settings_value val;
	GError *err = NULL;

	g_assert(section != NULL);
	g_assert(name != NULL);

	entry = settings_lookup(section, name, SETTINGS_UNKNOWN);

	if (!entry) {
		MSG_ERROR("Undefined settings entry %s/%s", section, name);
		return FALSE;
	}

	switch (entry->type) {
	case SETTINGS_NUM:
		val.num = g_key_file_get_integer(sonatina.rc, section, name, &err);
		break;
	case SETTINGS_BOOL:
		val.boolean = g_key_file_get_boolean(sonatina.rc, section, name, &err);
		break;
	case SETTINGS_STRING:
		val.string = g_key_file_get_string(sonatina.rc, section, name, &err);
		break;
	default:
		MSG_ERROR("Invalid settings entry type: %d", entry->type);
		return FALSE;
		break;
	}

	if (err) {
		MSG_ERROR("Couldn't retrieve settings entry %s/%s: %s", entry->section, entry->name, err->message);
		g_error_free(err);
		return FALSE;
	}

	*value = val;

	return TRUE;
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
		g_key_file_set_integer(sonatina.rc, section, name, value.num);
		break;
	case SETTINGS_BOOL:
		MSG_DEBUG("settings %s/%s = %d", section, name, value.boolean);
		g_key_file_set_boolean(sonatina.rc, section, name, value.boolean);
		break;
	case SETTINGS_STRING:
		g_assert(value.string != NULL);
		MSG_DEBUG("settings %s/%s = %s", section, name, value.string);
		g_key_file_set_string(sonatina.rc, section, name, value.string);
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
