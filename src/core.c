#include <glib.h>
#include <gtk/gtk.h>

#include <mpd/client.h>

#include "core.h"
#include "client.h"
#include "util.h"
#include "songattr.h"
#include "settings.h"

#include "playlist.h"
#include "library.h"
#include "gettext.h"

struct sonatina_instance sonatina;

void sonatina_init()
{
	struct sonatina_tab *tab;

	MSG_DEBUG("sonatina_init()");

	sonatina.mpdsource = NULL;
	sonatina_settings_load(&sonatina);
	sonatina_settings_default(sonatina.rc);

	sonatina.elapsed = 0;
	sonatina.total = 0;
	sonatina.counter = g_timer_new();
	g_timer_stop(sonatina.counter);
	g_timeout_add_seconds(1, counter_cb, NULL);

	sonatina.cur = -1;

	sonatina.tabs = NULL;

	tab = sonatina_tab_new("playlist", _("Playlist"), sizeof(struct pl_tab), pl_tab_init, pl_tab_set_source, pl_tab_destroy);
	sonatina_append_tab(tab);

	tab = sonatina_tab_new("library", _("Library"), sizeof(struct library_tab), library_tab_init, library_tab_set_source, library_tab_destroy);
	sonatina_append_tab(tab);
}

void sonatina_destroy()
{
	GList *cur;
	GObject *w;

	MSG_DEBUG("sonatina_destroy()");

	sonatina_settings_save();

	g_key_file_free(sonatina.rc);

	for (cur = sonatina.profiles; cur; cur = cur->next) {
		g_key_file_free(cur->data);
	}

	g_list_free(sonatina.profiles);

	w = gtk_builder_get_object(sonatina.gui, "window");
	gtk_widget_destroy(GTK_WIDGET(w));

	g_timer_destroy(sonatina.counter);
}

gboolean sonatina_connect(const char *host, int port)
{
	GMainContext *context;
	int mpdfd;
	GList *cur;
	struct sonatina_tab *tab;

	mpdfd = client_connect(host, port);
	if (mpdfd < 0) {
		MSG_ERROR("failed to connect to %s:%d", host, port);
		return FALSE;
	}

	context = g_main_context_default();
	sonatina.mpdsource = mpd_source_new(mpdfd);
	g_source_attach(sonatina.mpdsource, context);

	for (cur = sonatina.tabs; cur; cur = cur->next) {
		tab = cur->data;
		tab->set_mpdsource(tab, sonatina.mpdsource);
	}

	mpd_send(sonatina.mpdsource, MPD_CMD_STATUS, NULL);
	mpd_send(sonatina.mpdsource, MPD_CMD_PLINFO, NULL);
	mpd_send(sonatina.mpdsource, MPD_CMD_CURRENTSONG, NULL);

	return TRUE;
}

void sonatina_disconnect()
{
	GList *cur;
	struct sonatina_tab *tab;

	MSG_DEBUG("sonatina_disconnect()");

	if (!sonatina.mpdsource) {
		MSG_WARNING("sonatina_disconnect(): not connected");
		return;
	}

	for (cur = sonatina.tabs; cur; cur = cur->next) {
		tab = cur->data;
		tab->set_mpdsource(tab, NULL);
	}
	mpd_send(sonatina.mpdsource, MPD_CMD_CLOSE, NULL);
	mpd_source_close(sonatina.mpdsource);

	sonatina.mpdsource = NULL;
	sonatina.cur = -1;
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

	return TRUE;
}

void sonatina_update_song(const struct mpd_song *song)
{
	GObject *title;
	GObject *subtitle;
	gchar *format;
	gchar *str;
	int pos;

	title = gtk_builder_get_object(sonatina.gui, "title");
	subtitle = gtk_builder_get_object(sonatina.gui, "subtitle");

	format = g_key_file_get_string(sonatina.rc, "main", "title", NULL);
	str = song_attr_format(format, song);
	if (str) {
		gtk_label_set_text(GTK_LABEL(title), str);
	}
	g_free(format);
	g_free(str);

	format = g_key_file_get_string(sonatina.rc, "main", "subtitle", NULL);
	str = song_attr_format(format, song);
	if (str) {
		gtk_label_set_text(GTK_LABEL(subtitle), str);
	}
	 
	g_free(format);
	g_free(str);

	pos = mpd_song_get_pos(song);
	sonatina.cur = pos;
}

void sonatina_update_status(const struct mpd_status *status)
{
	GObject *w;
	int vol;
	enum mpd_state state;

	sonatina.cur = mpd_status_get_song_pos(status);

	/* volume */
	vol = mpd_status_get_volume(status);
	w = gtk_builder_get_object(sonatina.gui, "volbutton");
	if (vol >= 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(w), TRUE);
		gtk_scale_button_set_value(GTK_SCALE_BUTTON(w), vol/100.0);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(w), FALSE);
	}

	/* elapsed time */
	sonatina.elapsed = mpd_status_get_elapsed_time(status);
	sonatina.total = mpd_status_get_total_time(status);
	g_timer_start(sonatina.counter);
	g_timer_stop(sonatina.counter);

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
		g_timer_stop(sonatina.counter);
		break;
	case MPD_STATE_PLAY:
		gtk_widget_hide(GTK_WIDGET(play));
		gtk_widget_show(GTK_WIDGET(pause));
		g_timer_start(sonatina.counter);
		break;
	}
}

gboolean counter_cb(gpointer data)
{
	GObject *w;
	gchar *str;
	gdouble fraction;
	int elapsed;

	elapsed = sonatina.elapsed + g_timer_elapsed(sonatina.counter, NULL);

	if (sonatina.total <= 0 || elapsed < 0) {
		fraction = 0.0;
	} else {
		fraction = elapsed / ((double) sonatina.total);
	}
	w = gtk_builder_get_object(sonatina.gui, "timeline");
	str = g_strdup_printf("%d:%.2d / %d:%.2d", elapsed/60, elapsed%60, sonatina.total/60, sonatina.total%60);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w), fraction);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(w), str);
	g_free(str);

	return TRUE;
}

void sonatina_play(int pos)
{
	char buf[24];

	if (pos < 0) {
		mpd_send(sonatina.mpdsource, MPD_CMD_PLAY, NULL);
	} else {
		snprintf(buf, sizeof(buf) - 1, "%d", pos);
		mpd_send(sonatina.mpdsource, MPD_CMD_PLAY, buf, NULL);
	}
}

void sonatina_seek(int time)
{
	char buf[24];

	snprintf(buf, sizeof(buf) - 1, "%d", time);
	mpd_send(sonatina.mpdsource, MPD_CMD_SEEKCUR, buf, NULL);
}

void sonatina_setvol(double vol)
{
	char buf[24];

	snprintf(buf, sizeof(buf) - 1, "%d", (int) (vol * 100.0));
	mpd_send(sonatina.mpdsource, MPD_CMD_SETVOL, buf, NULL);
}


struct sonatina_tab *sonatina_tab_new(const char *name, const char *label, size_t size, TabInitFunc init, TabSetSourceFunc set_source, TabDestroyFunc destroy)
{
	struct sonatina_tab *tab;

	tab = g_malloc(size);
	tab->name = g_strdup(name);
	tab->label = g_strdup(label);
	tab->init = init;
	tab->set_mpdsource = set_source;
	tab->destroy = destroy;

	return tab;
}

void sonatina_tab_destroy(struct sonatina_tab *tab)
{
	tab->destroy(tab);
	g_free(tab->name);
	g_free(tab->label);
	g_free(tab);
}

gboolean sonatina_append_tab(struct sonatina_tab *tab)
{
	GObject *notebook;
	GtkWidget *label;

	if (!tab->init(tab)) {
		MSG_ERROR("Failed to append tab %s", tab->name);
		return FALSE;
	}

	sonatina.tabs = g_list_append(sonatina.tabs, tab);

	notebook = gtk_builder_get_object(sonatina.gui, "notebook");
	label = gtk_label_new(tab->label);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab->widget, label);

	return TRUE;
}

gboolean sonatina_remove_tab(const char *name)
{
	GList *cur;
	struct sonatina_tab *tab;
	gint i;
	GObject *notebook;

	for (cur = sonatina.tabs, i = 0; cur; cur = cur->next, i++) {
		tab = (struct sonatina_tab *) cur->data;
		if (!g_strcmp0(tab->name, name)) {
			notebook = gtk_builder_get_object(sonatina.gui, "notebook");
			gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), i);
			sonatina.tabs = g_list_remove_link(sonatina.tabs, cur);
			g_free(cur);
			sonatina_tab_destroy(tab);
			return TRUE;
		}
	}

	return FALSE;
}

