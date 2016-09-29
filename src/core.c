#include <gtk/gtk.h>

#include "core.h"
#include "client.h"
#include "util.h"

#define DEFAULT_ALBUM_ART SHAREDIR "/album_art.svg"

struct sonatina_instance sonatina;

void sonatina_init()
{
	gchar *str;

	sonatina.mpdsource = NULL;
	sonatina_settings_load(&sonatina);

	str = g_key_file_get_string(sonatina.rc, "main", "active_profile", NULL);
	if (str) {
		sonatina_change_profile(str);
		g_free(str);
	}
}

gboolean sonatina_connect(const char *host, int port)
{
	GMainContext *context;
	int mpdfd;

	mpdfd = client_connect(host, port);
	if (mpdfd < 0) {
		MSG_ERROR("failed to connect to %s:%d", host, port);
		return FALSE;
	}

	context = g_main_context_default();
	sonatina.mpdsource = mpd_source_new(mpdfd);
	g_source_attach(sonatina.mpdsource, context);

	return TRUE;
}

void sonatina_disconnect()
{
	mpd_send("close", NULL);
}

gboolean sonatina_change_profile(const char *new)
{
	GKeyFile *profile;
	gchar *host;
	gchar *name;
	gint port;

	profile = sonatina_get_profile(new);

	if (!profile) {
		return FALSE;
	}

	if (sonatina.mpdsource) {
		sonatina_disconnect();
	}

	name = g_key_file_get_string(profile, "profile", "name", NULL);
	host = g_key_file_get_string(profile, "profile", "host", NULL);
	port = g_key_file_get_integer(profile, "profile", "port", NULL);

	g_assert(name != NULL);
	g_assert(host != NULL);

	MSG_INFO("changing profile to %s", name);
	if (sonatina_connect(host, port)) {
		g_key_file_set_string(sonatina.rc, "main", "active_profile", name);
		g_free(name);
		g_free(host);
	}

	return TRUE;
}
