#include <gtk/gtk.h>

#include "core.h"
#include "client.h"
#include "util.h"

#define DEFAULT_ALBUM_ART SHAREDIR "/album_art.svg"

struct sonatina_instance sonatina;

void sonatina_init()
{
	gchar *str;

	MSG_DEBUG("sonatina_init()");

	sonatina.mpdsource = NULL;
	sonatina_settings_load(&sonatina);

	str = g_key_file_get_string(sonatina.rc, "main", "active_profile", NULL);
	if (str) {
		if (!sonatina_change_profile(str)) {
			MSG_WARNING("invalid profile %s", str);
		}
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

	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_STATUS, NULL);
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_CURRENTSONG, NULL);

	return TRUE;
}

void sonatina_disconnect()
{
	MSG_DEBUG("sonatina_disconnect()");

	if (!sonatina.mpdsource) {
		MSG_WARNING("sonatina_disconnect(): not connected");
		return;
	}
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_CLOSE, NULL);
	mpd_source_close(sonatina.mpdsource);
	sonatina.mpdsource = NULL;
}

gboolean sonatina_change_profile(const char *new)
{
	GKeyFile *profile;
	gchar *host;
	gchar *name;
	gint port;

	profile = sonatina_get_profile(new);

	if (!profile) {
		MSG_INFO("profile '%s' doesn't exist", new);
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

	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_STATUS, NULL);
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_CURRENTSONG, NULL);

	return TRUE;
}

void sonatina_update_song(const struct mpd_song *song)
{
	GObject *title;
	GObject *subtitle;
	const char *str;

	title = gtk_builder_get_object(sonatina.gui, "title");
	subtitle = gtk_builder_get_object(sonatina.gui, "subtitle");

	str = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
	if (str) {
		gtk_label_set_text(GTK_LABEL(title), str);
	}

	str = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
	if (str) {
		gtk_label_set_text(GTK_LABEL(subtitle), str);
	}
}

void sonatina_update_status(const struct mpd_status *status)
{
	GObject *w;
	int vol;
	enum mpd_state state;

	/* volume */
	vol = mpd_status_get_volume(status);
	w = gtk_builder_get_object(sonatina.gui, "volbutton");
	if (vol >= 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(w), TRUE);
		gtk_scale_button_set_value(GTK_SCALE_BUTTON(w), vol/100.0);
	}/* else {
		gtk_widget_set_sensitive(GTK_WIDGET(w), FALSE);
	}*/

	/* state */
	/* TODO: this is not an optimal solution as the butons blink while changing */
	GObject *play;
	GObject *pause;
	state = mpd_status_get_state(status);
	play = gtk_builder_get_object(sonatina.gui, "play_button");
	pause = gtk_builder_get_object(sonatina.gui, "pause_button");
	switch (state) {
	case MPD_STATE_UNKNOWN:
	case MPD_STATE_STOP:
	case MPD_STATE_PAUSE:
		gtk_widget_hide(GTK_WIDGET(pause));
		gtk_widget_show(GTK_WIDGET(play));
		break;
	case MPD_STATE_PLAY:
		gtk_widget_hide(GTK_WIDGET(play));
		gtk_widget_show(GTK_WIDGET(pause));
		break;
	}
}

