#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "settings.h"
#include "core.h"
#include "util.h"

gchar *rcdir;
gchar *rcfile;
gchar *profiledir;

void sonatina_settings_prepare()
{
	if (!rcdir) {
		rcdir = g_build_filename(g_get_user_config_dir(), PROG, NULL);
	}

	if (!rcfile) {
		rcfile = g_build_filename(g_get_user_config_dir(), PROG, "sonatinarc", NULL);
	}

	if (!profiledir) {
		profiledir = g_build_filename(g_get_user_config_dir(), PROG, "profiles", NULL);
	}

	g_mkdir_with_parents(profiledir, 0744);
}


gboolean sonatina_settings_load()
{
	gchar *path;
	gchar **profiles;
	gsize nprofiles, i;
	GKeyFile *profile;

	sonatina_settings_prepare();
	
	if (!sonatina.rc) {
		sonatina.rc = g_key_file_new();
	}

	g_key_file_load_from_file(sonatina.rc, rcfile,
	                                 G_KEY_FILE_KEEP_COMMENTS |
	                                 G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

	profiles = g_key_file_get_string_list(sonatina.rc, "main", "profiles", &nprofiles, NULL);

	if (!profiles) {
		MSG_INFO("no profiles set; adding default");
		sonatina_add_profile("localhost", "localhost", 6600);
		profiles = g_key_file_get_string_list(sonatina.rc, "main", "profiles", &nprofiles, NULL);
	}

	sonatina.profiles = NULL;

	for (i = 0; i < nprofiles; i++) {
		path = g_build_filename(profiledir, profiles[i], NULL);
		MSG_INFO("loading profile '%s'", path);
		profile = g_key_file_new();
		g_key_file_load_from_file(profile, path,
		                          G_KEY_FILE_KEEP_COMMENTS |
		                          G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
		sonatina.profiles = g_list_append(sonatina.profiles, profile);
	}

	g_strfreev(profiles);

	return TRUE;
}

void sonatina_add_profile(const char *name, const char *host, int port)
{
	gchar *path;
	GKeyFile *profile;

	path = g_build_filename(profiledir, name, NULL);

	profile = g_key_file_new();
	g_key_file_load_from_file(profile, path,
	                          G_KEY_FILE_KEEP_COMMENTS |
	                          G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
	g_key_file_set_string(profile, "profile", "name", name);
	g_key_file_set_string(profile, "profile", "host", host);
	g_key_file_set_integer(profile, "profile", "port", port);

	sonatina.profiles = g_list_append(sonatina.profiles, profile);
}

GKeyFile *sonatina_get_profile(const char *name)
{
	GList *cur;
	GKeyFile *profile;

	for (cur = sonatina.profiles; cur; cur = cur->next) {
		profile = (GKeyFile *) cur->data;
		if (!g_strcmp0(g_key_file_get_string(profile, "profile", "name", NULL), name)) {
			return profile;
		}
	}

	return NULL;
}


