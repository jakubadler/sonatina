#include <glib.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "client.h"
#include "core.h"
#include "util.h"

#define UIFILE DATADIR "/" PROG "/" PROG ".ui"

void app_startup_cb(GtkApplication *app, gpointer user_data)
{
	GObject *win;
	GObject *w;
	gchar *str;

	MSG_DEBUG("app_startup_cb()");

	sonatina.gui = gtk_builder_new();
	gtk_builder_add_from_file(sonatina.gui, UIFILE, NULL);

	sonatina_init();

	win = gtk_builder_get_object(sonatina.gui, "window");
	g_signal_connect(win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	gtk_application_add_window(app, GTK_WINDOW(win));

	/* playlist TreeView */
	str = g_key_file_get_string(sonatina.rc, "playlist", "format", NULL);
	pl_init(&sonatina.pl, str);
	g_free(str);

	w = gtk_builder_get_object(sonatina.gui, "playlist_tw");
	gtk_tree_view_set_model(GTK_TREE_VIEW(w), GTK_TREE_MODEL(sonatina.pl.store));

	connect_signals();

	/* keep application running after main window is closed */
	g_application_hold(G_APPLICATION(app));
}

void app_shutdown_cb(GtkApplication *app, gpointer user_data)
{
	MSG_DEBUG("app_shutdown_cb()");
	sonatina_disconnect();
	sonatina_destroy();
}

void app_activate_cb(GtkApplication *app, gpointer user_data)
{
	GObject *win;

	MSG_DEBUG("app_activate_cb()");

	win = gtk_builder_get_object(sonatina.gui, "window");
	gtk_widget_show_all(GTK_WIDGET(win));
}

gint app_options_cb(GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data)
{
	GVariantDict *dict;
	GVariant *value;
	gchar *str;

	dict = g_application_command_line_get_options_dict(cmdline);

	value = g_variant_dict_lookup_value(dict, "kill", NULL);
	if (value) {
		MSG_DEBUG("--kill");
		g_application_quit(app);
	}

	if (g_variant_dict_lookup(dict, "profile", "s", &str)) {
		MSG_DEBUG("--profile %s", str);
		sonatina_change_profile(str);
		g_free(str);
	}

	g_application_activate(app);

	return 0;
}

gint app_local_options_cb(GApplication *app, GVariantDict *dict, gpointer user_data)
{
	GVariant *value;

	value = g_variant_dict_lookup_value(dict, "verbose", NULL);
	if (value) {
		MSG_DEBUG("--verbose");
		log_level = LEVEL_INFO;
	}

	return -1;
}

void prev_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("prev_cb");
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_PREV, NULL);
}

void next_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("next_cb");
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_NEXT, NULL);
}

void play_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("play_cb");
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_PLAY, NULL);
}

void pause_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("pause_cb");
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_PAUSE, NULL);
}

void stop_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("stop_cb");
	mpd_send_cmd(sonatina.mpdsource, MPD_CMD_STOP, NULL);
}

void volume_cb(GtkWidget *w, gpointer data)
{
	gdouble scale;

	MSG_DEBUG("volume_cb");
	scale = gtk_scale_button_get_value(GTK_SCALE_BUTTON(w));
	sonatina_setvol(scale);
}

gboolean timeline_clicked_cb(GtkWidget *w, GdkEvent *event, gpointer data)
{
	gdouble x;
	int width, time;

	MSG_DEBUG("timeline_clicked_cb()");

	x = ((GdkEventButton *) event)->x;
	width = gtk_widget_get_allocated_width(w);

	if (width == 0) time = 0;
	else time = (x / width) * sonatina.total;
	sonatina_seek(time);

	return FALSE;
}

void playlist_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	int pos;

	store = gtk_tree_view_get_model(tw);

	if (!gtk_tree_model_get_iter(store, &iter, path)) {
		MSG_WARNING("playlist item vanished");
		return;
	}

	gtk_tree_model_get(store, &iter, PL_POS, &pos, -1);

	sonatina_play(pos);
}

void connect_signals()
{
	GObject *w;

	/* playback buttons */
	w = gtk_builder_get_object(sonatina.gui, "prev_button");
	g_signal_connect(w, "clicked", G_CALLBACK(prev_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "play_button");
	g_signal_connect(w, "clicked", G_CALLBACK(play_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "pause_button");
	g_signal_connect(w, "clicked", G_CALLBACK(pause_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "stop_button");
	g_signal_connect(w, "clicked", G_CALLBACK(stop_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "next_button");
	g_signal_connect(w, "clicked", G_CALLBACK(next_cb), NULL);

	/* volume button */
	w = gtk_builder_get_object(sonatina.gui, "volbutton");
	g_signal_connect(w, "value-changed", G_CALLBACK(volume_cb), NULL);

	/* timeline */
	w = gtk_builder_get_object(sonatina.gui, "timeline_eb");
	gtk_widget_add_events(GTK_WIDGET(w), GDK_BUTTON_PRESS_MASK);
	g_signal_connect(w, "button-press-event", G_CALLBACK(timeline_clicked_cb), NULL);

	/* playlist */
	w = gtk_builder_get_object(sonatina.gui, "playlist_tw");
	g_signal_connect(w, "row-activated", G_CALLBACK(playlist_clicked_cb), NULL);
}

