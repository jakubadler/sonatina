#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <glib.h>

#include "util.h"
#include "gettext.h"

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

struct mpd_tag_entity *mpd_tag_entity_begin(const struct mpd_pair *pair)
{
	struct mpd_tag_entity *entity;

	entity = g_malloc(sizeof(struct mpd_tag_entity));

	entity->genre = NULL;
	entity->artist = NULL;
	entity->album = NULL;
	entity->date = NULL;

	mpd_tag_entity_feed(entity, pair);

	return entity;
}

gboolean mpd_tag_entity_feed(struct mpd_tag_entity *entity, const struct mpd_pair *pair)
{
	g_assert(entity != NULL);
	g_assert(pair != NULL);

	if (!g_strcmp0(pair->name, "Genre")) {
		if (!entity->genre) {
			entity->genre = g_strdup(pair->value);
			return TRUE;
		}
	} else if (!g_strcmp0(pair->name, "Artist")) {
		if (!entity->artist) {
			entity->artist = g_strdup(pair->value);
			return TRUE;
		}
	} else if (!g_strcmp0(pair->name, "AlbumArtist")) {
		if (!entity->artist) {
			entity->artist = g_strdup(pair->value);
			return TRUE;
		}
	} else if (!g_strcmp0(pair->name, "Album")) {
		if (!entity->album) {
			entity->album = g_strdup(pair->value);
			return TRUE;
		}
	} else if (!g_strcmp0(pair->name, "Date")) {
		if (!entity->date) {
			entity->date = g_strdup(pair->value);
			return TRUE;
		}
	}

	return FALSE;
}

void mpd_tag_entity_free(struct mpd_tag_entity *entity)
{
	if (entity->genre) {
		g_free(entity->genre);
	}

	if (entity->artist) {
		g_free(entity->artist);
	}

	if (entity->album) {
		g_free(entity->album);
	}

	if (entity->date) {
		g_free(entity->date);
	}

	g_free(entity);
}

