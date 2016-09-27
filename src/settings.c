#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "settings.h"
#include "core.h"

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
	sonatina_settings_prepare();
	
	if (!sonatina.rc) {
		sonatina.rc = g_key_file_new();
	}

	return g_key_file_load_from_file(sonatina.rc, rcfile,
	                                 G_KEY_FILE_KEEP_COMMENTS |
	                                 G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
}

void sonatina_settings_default(struct sonatina_settings *s)
{
	s->art_size = 128;
}

gboolean sonatina_settings_apply()
{
	return TRUE;
}

