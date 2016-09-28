#include <glib.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "client.h"
#include "core.h"
#include "util.h"

void app_activate_cb(GtkApplication *app, gpointer user_data)
{
	GObject *win;

	sonatina_init();

	sonatina.gui = gtk_builder_new();
	gtk_builder_add_from_file(sonatina.gui, SHAREDIR "/" PROG ".ui", NULL);

	win = gtk_builder_get_object(sonatina.gui, "window");
	gtk_application_add_window(app, GTK_WINDOW(win));
	gtk_widget_show_all(GTK_WIDGET(win));

	connect_signals();
	sonatina_connect("localhost", 6600);
}

void prev_cb(GtkWidget *w, gpointer data)
{
	puts("prev_cb");
	mpd_send("previous", NULL);
}

void next_cb(GtkWidget *w, gpointer data)
{
	puts("next_cb");
	mpd_send("next", NULL);
}

void play_cb(GtkWidget *w, gpointer data)
{
	puts("play_cb");
	mpd_send("play", NULL);
}

void stop_cb(GtkWidget *w, gpointer data)
{
	puts("stop_cb");
	mpd_send("stop", NULL);
}

void volume_cb(GtkWidget *w, gpointer data)
{
	gdouble scale;
	char buf[16];

	puts("volume_cb");
	scale = gtk_scale_button_get_value(GTK_SCALE_BUTTON(w));
	sprintf(buf, "%d", (int) (scale * 100.0));
	mpd_send("setvol", buf, NULL);
}

void connect_signals()
{
	GObject *w;

	/* playback buttons */
	w = gtk_builder_get_object(sonatina.gui, "prev_button");
	g_signal_connect(w, "clicked", G_CALLBACK(prev_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "play_button");
	g_signal_connect(w, "clicked", G_CALLBACK(play_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "stop_button");
	g_signal_connect(w, "clicked", G_CALLBACK(stop_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "next_button");
	g_signal_connect(w, "clicked", G_CALLBACK(next_cb), NULL);

	/* volume button */
	w = gtk_builder_get_object(sonatina.gui, "volbutton");
	g_signal_connect(w, "value-changed", G_CALLBACK(volume_cb), NULL);
}

