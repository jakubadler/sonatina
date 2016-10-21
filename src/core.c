#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <mpd/client.h>

#include "core.h"
#include "client.h"
#include "util.h"
#include "songattr.h"

#define DEFAULT_ALBUM_ART SHAREDIR "/album_art.svg"

struct sonatina_instance sonatina;

void sonatina_init()
{
	gchar *str;

	MSG_DEBUG("sonatina_init()");

	sonatina.mpdsource = NULL;
	sonatina_settings_load(&sonatina);
	sonatina_settings_default(sonatina.rc);

	str = g_key_file_get_string(sonatina.rc, "main", "active_profile", NULL);
	if (str) {
		if (!sonatina_change_profile(str)) {
			MSG_WARNING("invalid profile %s", str);
		}
		g_free(str);
	}

	sonatina.elapsed = 0;
	sonatina.total = 0;
	sonatina.counter = g_timer_new();
	g_timer_stop(sonatina.counter);
	g_timeout_add_seconds(1, counter_cb, NULL);

	sonatina.cur = -1;
}

void sonatina_destroy()
{
	GList *cur;
	GObject *w;

	MSG_DEBUG("sonatina_destroy()");

	sonatina_settings_save();

	pl_free(&sonatina.pl);
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
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_PLINFO, NULL);

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
	sonatina.cur = -1;
	sonatina_update_pl(NULL);
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
	pl_set_active(&sonatina.pl, pos);
}

void sonatina_update_status(const struct mpd_status *status)
{
	GObject *w;
	int vol;
	enum mpd_state state;
	int cur;

	cur = mpd_status_get_song_pos(status);
	if (cur != sonatina.cur) {
		/* song changed */
		mpd_send_cmd(sonatina.mpdsource, MPD_CMD_CURRENTSONG, NULL);
	}
	pl_set_active(&sonatina.pl, sonatina.cur);

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

void sonatina_update_pl(const struct mpd_song *song)
{
	pl_update(&sonatina.pl, song);
}

void sonatina_clear_pl()
{
	gtk_list_store_clear(sonatina.pl.store);
}

gboolean pl_init(struct playlist *pl, const char *format)
{
	size_t i;
	GType *types;
	GObject *tw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	gchar *title;

	pl->n_columns = 1;
	for (i = 0; format[i]; i++) {
		if (format[i] == '|')
			pl->n_columns++;
	}

	pl->columns = g_strsplit(format, "|", 0);
	types = g_malloc((PL_COUNT + pl->n_columns) * sizeof(GType));

	types[PL_ID] = G_TYPE_INT;
	types[PL_POS] = G_TYPE_INT;
	types[PL_WEIGHT] = G_TYPE_INT;
	for (i = 0; pl->columns[i]; i++) {
		types[PL_COUNT + i] = G_TYPE_STRING;
	}
	pl->store = gtk_list_store_newv(PL_COUNT + pl->n_columns, types);
	g_free(types);

	/* update TreeView */
	tw = gtk_builder_get_object(sonatina.gui, "playlist_tw");

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "weight-set", TRUE, NULL);

	/* remove all columns */
	for (i = 0; (col = gtk_tree_view_get_column(GTK_TREE_VIEW(tw), i)); i++) {
		gtk_tree_view_remove_column(GTK_TREE_VIEW(tw), col);
	}

	/* add new columns */
	for (i = 0; pl->columns[i]; i++) {
		title = song_attr_format(pl->columns[i], NULL);
		MSG_DEBUG("adding column with format '%s' to playlist tw", pl->columns[i]);
		col = gtk_tree_view_column_new_with_attributes(title, renderer, "weight", PL_WEIGHT, "text", PL_COUNT + i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tw), col);
	}

	return TRUE;
}

void pl_set_active(struct playlist *pl, int pos_req)
{
	GtkTreeIter iter;
	gint id, pos;

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl->store), &iter)) {
		/* empty playlist */
		return;
	}

	do {
		gtk_tree_model_get(GTK_TREE_MODEL(pl->store), &iter, PL_ID, &id, PL_POS, &pos, -1);
		if (pos == pos_req) {
			gtk_list_store_set(pl->store, &iter, PL_WEIGHT, PANGO_WEIGHT_BOLD, -1);
		} else {
			gtk_list_store_set(pl->store, &iter, PL_WEIGHT, PANGO_WEIGHT_NORMAL, -1);
		}
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl->store), &iter));
}

void pl_update(struct playlist *pl, const struct mpd_song *song)
{
	int id, pos;
	GtkTreePath *path;
	GtkTreeIter iter;
	size_t i;
	gchar *str;

	if (!song) {
		gtk_list_store_clear(pl->store);
		return;
	}

	id = mpd_song_get_id(song);
	pos = mpd_song_get_pos(song);
	path = gtk_tree_path_new_from_indices(pos, -1);
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(pl->store), &iter, path)) {
		gtk_list_store_insert(pl->store, &iter, pos);
	}
	gtk_tree_path_free(path);

	gtk_list_store_set(pl->store, &iter, PL_ID, id, PL_POS, pos, PL_WEIGHT, PANGO_WEIGHT_NORMAL, -1);

	for (i = 0; i < pl->n_columns; i++) {
		str = song_attr_format(pl->columns[i], song);
		gtk_list_store_set(pl->store, &iter, PL_COUNT + i, str, -1);
		g_free(str);
	}
}

void pl_free(struct playlist *pl)
{
	g_strfreev(pl->columns);
	gtk_list_store_clear(pl->store);
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
		mpd_send_cmd(sonatina.mpdsource, MPD_CMD_PLAY, NULL);
	} else {
		snprintf(buf, sizeof(buf) - 1, "%d", pos);
		mpd_send_cmd(sonatina.mpdsource, MPD_CMD_PLAY, buf, NULL);
	}
}

void sonatina_seek(int time)
{
	char buf[24];

	snprintf(buf, sizeof(buf) - 1, "%d", time);
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_SEEKCUR, buf, NULL);
}

void sonatina_setvol(double vol)
{
	char buf[24];

	snprintf(buf, sizeof(buf) - 1, "%d", (int) (vol * 100.0));
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_SETVOL, buf, NULL);
}

