#include <locale.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "gettext.h"

int main(int argc, char **argv)
{
	GtkApplication *app;

#if ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	app = gtk_application_new("org.eu.cz.adl.sonatina", G_APPLICATION_HANDLES_COMMAND_LINE);
	g_signal_connect(app, "startup", G_CALLBACK(app_startup_cb), NULL);
	g_signal_connect(app, "shutdown", G_CALLBACK(app_shutdown_cb), NULL);
	g_signal_connect(app, "activate", G_CALLBACK(app_activate_cb), NULL);
	g_signal_connect(app, "command-line", G_CALLBACK(app_options_cb), NULL);
	g_signal_connect(app, "handle-local-options", G_CALLBACK(app_local_options_cb), NULL);

	g_application_add_main_option(G_APPLICATION(app),
			"verbose", 'v',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Increase verbosity level"), NULL);
	g_application_add_main_option(G_APPLICATION(app),
			"kill", 'k',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Stop currently running sonatina instance"), NULL);
	g_application_add_main_option(G_APPLICATION(app),
			"profile", 'p',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_STRING,
			_("Change MPD profile"), "PROFILE");
	g_application_run(G_APPLICATION(app), argc, argv);

	return 0;
}

