#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "util.h"
#include "gui.h"
#include "client.h"

int main(int argc, char **argv)
{
	GtkApplication *app;

	app = gtk_application_new("org.eu.cz.adl.sonatina", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(app_activate_cb), NULL);
	g_application_run(G_APPLICATION(app), argc, argv);

	return 0;
}

