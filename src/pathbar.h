#ifndef SONATINA__PATHBAR_H
#define SONATINA__PATHBAR_H

#include <glib-object.h>
#include <gtk/gtk.h>

/*
 * Potentially, include other headers on which this header depends.
 */

G_BEGIN_DECLS

/*
 * Type declaration.
 */
#define SONATINA_TYPE_PATH_BAR sonatina_path_bar_get_type()
G_DECLARE_FINAL_TYPE(SonatinaPathBar, sonatina_path_bar, SONATINA, PATH_BAR, GtkBox)

/*
 * Method definitions.
 */
SonatinaPathBar *sonatina_path_bar_new(void);
void sonatina_path_bar_open(SonatinaPathBar *self, const gchar *name, const gchar *icon);
void sonatina_path_bar_open_root(SonatinaPathBar *self, const gchar *name, const gchar *icon);

G_END_DECLS

#endif
