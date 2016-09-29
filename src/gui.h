#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

void app_activate_cb(GtkApplication *app, gpointer user_data);
gint app_options_cb(GApplication *app, GVariantDict *dict, gpointer user_data);

void prev_cb(GtkWidget *w, gpointer data);
void next_cb(GtkWidget *w, gpointer data);
void play_cb(GtkWidget *w, gpointer data);
void stop_cb(GtkWidget *w, gpointer data);

void connect_signals();


#endif
