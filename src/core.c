#include <glib.h>
#include <gtk/gtk.h>

#include <mpd/client.h>

#include "core.h"
#include "client.h"
#include "util.h"
#include "songattr.h"
#include "settings.h"
#include "profile.h"

#include "playlist.h"
#include "library.h"
#include "gui.h"
#include "gettext.h"

struct sonatina_instance sonatina;

void sonatina_init()
{
	struct sonatina_tab *tab;

	MSG_DEBUG("sonatina_init()");

	sonatina.mpdsource = NULL;
	sonatina_settings_load(&sonatina);

	sonatina_profiles_load();

	sonatina.elapsed_ms = 0;
	sonatina.total = 0;
	sonatina.counter = g_timer_new();
	g_timer_stop(sonatina.counter);
	g_timeout_add(200, counter_cb, NULL);

	sonatina.cur = -1;

	sonatina.tabs = NULL;

	tab = sonatina_tab_new("playlist", _("Playlist"), sizeof(struct pl_tab), pl_tab_init, pl_tab_set_source, pl_tab_destroy);
	sonatina_append_tab(tab);

	tab = sonatina_tab_new("library", _("Library"), sizeof(struct library_tab), library_tab_init, library_tab_set_source, library_tab_destroy);
	sonatina_append_tab(tab);
}

void sonatina_destroy()
{
	MSG_DEBUG("sonatina_destroy()");

	sonatina_settings_save();
	sonatina_profiles_save();
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

	mpd_source_register(sonatina.mpdsource, MPD_CMD_STATUS, sonatina_update_status, NULL);
	mpd_source_register(sonatina.mpdsource, MPD_CMD_CURRENTSONG, sonatina_update_song, NULL);

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

	for (cur = sonatina.tabs; cur; cur = cur->next) {
		tab = cur->data;
		tab->set_mpdsource(tab, NULL);
	}

	if (!sonatina.mpdsource) {
		MSG_WARNING("sonatina_disconnect(): not connected");
		return;
	}

	mpd_send(sonatina.mpdsource, MPD_CMD_CLOSE, NULL);
	mpd_source_close(sonatina.mpdsource);

	sonatina.mpdsource = NULL;
	sonatina.cur = -1;
	g_timer_stop(sonatina.counter);

	sonatina_set_labels(_("Sonatina"), _("Disconnected"));
	remove_connected_entries();
}

gboolean sonatina_change_profile(const struct sonatina_profile *profile)
{
	union settings_value val;

	if (sonatina.mpdsource) {
		sonatina_disconnect();
	}

	if (!profile) {
		return TRUE;
	}

	MSG_INFO("changing profile to %s", profile->name);
	if (sonatina_connect(profile->host, profile->port)) {
		val.string = profile->name;
		sonatina_settings_set("main", "active_profile", val);
	}

	add_connected_entries();

	return TRUE;
}

gboolean sonatina_change_profile_by_name(const char *new)
{
	const struct sonatina_profile *profile;

	if (!new) {
		return sonatina_change_profile(NULL);
	}

	profile = sonatina_get_profile(new);

	if (!profile) {
		MSG_INFO("profile '%s' doesn't exist", new);
		return FALSE;
	}

	return sonatina_change_profile(profile);
}

void sonatina_update_song(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	const struct mpd_song *song = answer->song;
	gchar *format;
	gchar *str;
	int pos;

	if (song) {
		format = sonatina_settings_get_string("main", "title");
		str = song_attr_format(format, song);
		if (str) {
			sonatina_set_labels(str, NULL);
		}
		g_free(format);
		g_free(str);

		format = sonatina_settings_get_string("main", "subtitle");
		str = song_attr_format(format, song);
		if (str) {
			sonatina_set_labels(NULL, str);
		}
		 
		g_free(format);
		g_free(str);

		pos = mpd_song_get_pos(song);
		sonatina.cur = pos;
	} else {
		sonatina_set_labels(_("Sonatina"), _("Stopped"));
	}
}

void sonatina_update_status(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	const struct mpd_status *status = answer->status;
	GObject *w;
	int vol;
	enum mpd_state state;
	const char *mpd_err;
	GApplication *app;
	GAction *action;

	if (!status) {
		return;
	}

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
	sonatina.elapsed_ms = mpd_status_get_elapsed_ms(status);
	sonatina.total = mpd_status_get_total_time(status);
	g_timer_start(sonatina.counter);
	g_timer_stop(sonatina.counter);

	/* state */
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

	mpd_err = mpd_status_get_error(status);
	if (mpd_err) {
		MSG_ERROR("MPD: %s", mpd_err);
	}

	app = g_application_get_default();

	if (!app) {
		return;
	}

	action = g_action_map_lookup_action(G_ACTION_MAP(app), "repeat");
	if (action) {
		g_action_change_state(action, g_variant_new("b", mpd_status_get_repeat(status)));
	}

	action = g_action_map_lookup_action(G_ACTION_MAP(app), "random");
	if (action) {
		g_action_change_state(action, g_variant_new("b", mpd_status_get_random(status)));
	}

	action = g_action_map_lookup_action(G_ACTION_MAP(app), "single");
	if (action) {
		g_action_change_state(action, g_variant_new("b", mpd_status_get_single(status)));
	}

	action = g_action_map_lookup_action(G_ACTION_MAP(app), "consume");
	if (action) {
		g_action_change_state(action, g_variant_new("b", mpd_status_get_consume(status)));
	}
}

gboolean counter_cb(gpointer data)
{
	GObject *w;
	gchar *str;
	gdouble fraction;
	int elapsed_ms;

	elapsed_ms = sonatina.elapsed_ms + g_timer_elapsed(sonatina.counter, NULL)*1000.0;

	if (sonatina.total <= 0 || elapsed_ms < 0) {
		fraction = 0.0;
	} else {
		fraction = elapsed_ms / (((double) sonatina.total) * 1000.0);
	}
	w = gtk_builder_get_object(sonatina.gui, "timeline");
	str = g_strdup_printf("%d:%.2d / %d:%.2d", (elapsed_ms/1000)/60, (elapsed_ms/1000)%60, sonatina.total/60, sonatina.total%60);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w), fraction);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(w), str);
	g_free(str);

	return TRUE;
}

void sonatina_play(int pos)
{
	char buf[INT_BUF_SIZE];

	if (pos < 0) {
		mpd_send(sonatina.mpdsource, MPD_CMD_PLAY, NULL);
	} else {
		snprintf(buf, sizeof(buf), "%d", pos);
		mpd_send(sonatina.mpdsource, MPD_CMD_PLAY, buf, NULL);
	}
}

void sonatina_seek(int time)
{
	char buf[INT_BUF_SIZE];

	snprintf(buf, sizeof(buf), "%d", time);
	mpd_send(sonatina.mpdsource, MPD_CMD_SEEKCUR, buf, NULL);
}

void sonatina_setvol(double vol)
{
	char buf[INT_BUF_SIZE];

	snprintf(buf, sizeof(buf), "%d", (int) (vol * 100.0));
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

