#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "client.h"

void print_help(const char *name)
{
	printf("Usage: %s [-H HOST] [-p PORT]\n", name);
}

int main(int argc, char **argv)
{
	int opt;
	const char *host = "127.0.0.1";
	const char *port = "6600";

	while ((opt = getopt(argc, argv, "hH:p:")) != -1) {
		switch (opt) {
		case 'H':
			host = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		default:
			print_help(argv[0]);
			exit(1);
		}
	}

	printf("host: %s\nport: %s\n", host, port);

	GtkApplication *app;

	app = gtk_application_new("org.eu.cz.adl.sonatina", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(app_activate_cb), NULL);
	g_application_run(G_APPLICATION(app), argc, argv);

	return 0;
}

