#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "settings.h"


gboolean sonatina_settings_load(struct sonatina_settings *settings, const char *path)
{
	return TRUE;
}

void sonatina_settings_default(struct sonatina_settings *s)
{
	s->art_size = 128;
}

gboolean sonatina_settings_apply()
{
	return TRUE;
}

