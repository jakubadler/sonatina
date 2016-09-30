#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

void app_startup_cb(GtkApplication *app, gpointer user_data);
void app_shutdown_cb(GtkApplication *app, gpointer user_data);
void app_activate_cb(GtkApplication *app, gpointer user_data);
gint app_options_cb(GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data);
gint app_local_options_cb(GApplication *app, GVariantDict *dict, gpointer user_data);

void prev_cb(GtkWidget *w, gpointer data);
void next_cb(GtkWidget *w, gpointer data);
void play_cb(GtkWidget *w, gpointer data);
void stop_cb(GtkWidget *w, gpointer data);

void connect_signals();


#endif
