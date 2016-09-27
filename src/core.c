#include <gtk/gtk.h>
#include <mpd/async.h>

#include "core.h"
#include "client.h"

#define DEFAULT_ALBUM_ART SHAREDIR "/album_art.svg"

struct sonatina_instance sonatina;

void sonatina_init()
{
	sonatina.mpdsource = NULL;

	sonatina_settings_load(&sonatina);
	sonatina_settings_apply();
}

void sonatina_connect(const char *host, int port)
{
	GMainContext *context;
	int mpdfd;

	mpdfd = client_connect(host, port);
	if (mpdfd < 0) {
		fputs("failed to connect", stderr);
		return;
	}

	context = g_main_context_default();
	sonatina.mpdsource = mpd_source_new(mpdfd);
	g_source_attach(sonatina.mpdsource, context);
}

void sonatina_disconnect()
{
	mpd_send("close", NULL);
}
