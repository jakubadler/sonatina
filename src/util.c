#include <stdio.h>
#include <stdarg.h>

#include "util.h"

#ifdef DEBUG
int log_level = LEVEL_DEBUG;
#else
int log_level = LEVEL_WARNING;
#endif

int printmsg(int level, const char *format, ...)
{
	FILE *out;
	va_list ap;

	out = stderr;

	if (level < log_level) {
		return 0;
	}

	va_start(ap, format);
	switch (level) {
	case LEVEL_DEBUG:
		fprintf(out, "DEBUG: ");
		break;
	case LEVEL_INFO:
		fprintf(out, "INFO: ");
		break;
	case LEVEL_WARNING:
		fprintf(out, "WARNING: ");
		break;
	case LEVEL_ERROR:
		fprintf(out, "ERROR: ");
		break;
	case LEVEL_FATAL:
	default:
		fprintf(out, "FATAL: ");
		break;
	}
	vfprintf(out, format, ap);
	fprintf(out, "\n");
	va_end(ap);

	return 0;
}

GtkBuilder *load_tab_ui(const char *name)
{
	GtkBuilder *ui;
	gchar *uifile;
	gchar *uipath;
	gint err;

	uifile = g_strdup_printf("%s.ui", name);
	uipath = g_build_filename(DATADIR, PACKAGE, uifile, NULL);
	g_free(uifile);

	ui = gtk_builder_new();
	MSG_INFO("loading ui file %s", uipath);
	err = gtk_builder_add_from_file(ui, uipath, NULL);
	g_free(uipath);

	if (err == 0) {
		g_object_unref(G_OBJECT(ui));
		return NULL;
	}

	return ui;
}
