#include <glib.h>
#include <gtk/gtk.h>

#include "library.h"
#include "util.h"


gboolean library_tab_init(struct sonatina_tab *tab)
{
	struct library_tab *libtab = (struct library_tab *) tab;

	libtab->ui = load_tab_ui(tab->name);
	if (!libtab->ui) {
		return FALSE;
	}

	/* set tab widget */
	tab->widget = GTK_WIDGET(gtk_builder_get_object(libtab->ui, "top"));

	return TRUE;
}

void library_tab_set_source(struct sonatina_tab *tab, GSource *source)
{
}

void library_tab_destroy(struct sonatina_tab *tab)
{
	struct library_tab *libtab = (struct library_tab *) tab;

	g_object_unref(G_OBJECT(libtab->ui));
}

void library_path_display_dir(struct library_path *path, GtkListStore *model)
{
}

