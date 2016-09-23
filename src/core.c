#include <gtk/gtk.h>
#include <mpd/async.h>

#include "core.h"
#include "client.h"

#define DEFAULT_ALBUM_ART SHAREDIR "/album_art.svg"

struct sonatina_instance sonatina;

void sonatina_init()
{
	sonatina.mpdsource = NULL;

	sonatina_settings_default(&sonatina.settings);
	sonatina_settings_load(&sonatina.settings, SETTINGS_FILE);
	sonatina_settings_apply();

	//sonatina_change_art(DEFAULT_ALBUM_ART);
}


void sonatina_change_art(const char *path)
{
	GObject *img;
	GdkPixbuf *pixbuf;
	GError *error;

	pixbuf = gdk_pixbuf_new_from_file_at_size(path, sonatina.settings.art_size, sonatina.settings.art_size, &error);
	if (error) {
		fprintf(stderr, "Failed to load image '%s': %s\n", path, error->message);
		g_error_free(error);
	}
	if (!pixbuf) {
		return;
	}

	img = gtk_builder_get_object(sonatina.gui, "album_art");
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pixbuf);
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
