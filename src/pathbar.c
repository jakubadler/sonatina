#include "pathbar.h"

typedef struct {
	gint selected;
} SonatinaPathBarPrivate;

struct _SonatinaPathBar
{
  GtkBox parent_instance;
};

G_DEFINE_TYPE_WITH_PRIVATE(SonatinaPathBar, sonatina_path_bar, GTK_TYPE_BOX)

void path_bar_toggled(GtkToggleButton *button, gpointer data);

enum {
	CHANGED,
	N_SIGNALS
};

static int path_bar_signals[N_SIGNALS];

static void sonatina_path_bar_class_init(SonatinaPathBarClass *class)
{
	path_bar_signals[CHANGED] = g_signal_new("changed",
			G_TYPE_FROM_CLASS(class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			0 /* closure */,
			NULL /* accumulator */,
			NULL /* accumulator data */,
			NULL /* C marshaller */,
			G_TYPE_NONE /* return_type */,
			1     /* n_params */,
			G_TYPE_INT,
			NULL  /* param_types */);
}

static void sonatina_path_bar_init(SonatinaPathBar *self)
{
	SonatinaPathBarPrivate *priv = sonatina_path_bar_get_instance_private(self);

	priv->selected = -1;
	gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_HORIZONTAL);
}

SonatinaPathBar *sonatina_path_bar_new(void)
{
	return SONATINA_PATH_BAR(g_object_new(SONATINA_TYPE_PATH_BAR, NULL));
}

void sonatina_path_bar_open(SonatinaPathBar *self, const gchar *name, const gchar *icon)
{
	SonatinaPathBarPrivate *priv = sonatina_path_bar_get_instance_private(self);
	GtkWidget *button;
	GtkWidget *img;
	GList *children;
	GList *cur;
	gint i;

	children = gtk_container_get_children(GTK_CONTAINER(self));

	for (cur = children, i = 0; cur; cur = cur->next, i++) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cur->data), FALSE);
		if (i > priv->selected) {
			gtk_widget_destroy(GTK_WIDGET(cur->data));
		}
	}
	g_list_free(children);

	button = gtk_toggle_button_new_with_label(name);
	if (icon) {
		img = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image(GTK_BUTTON(button), img);
	}
	gtk_box_pack_start(GTK_BOX(self), button, FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(self));
	gtk_container_child_get(GTK_CONTAINER(self), GTK_WIDGET(button), "position", &priv->selected, NULL);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(path_bar_toggled), self);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
}

void sonatina_path_bar_open_root(SonatinaPathBar *self, const gchar *name, const gchar *icon)
{
	SonatinaPathBarPrivate *priv = sonatina_path_bar_get_instance_private(self);

	priv->selected = -1;
	sonatina_path_bar_open(self, name, icon);
}

void path_bar_toggled(GtkToggleButton *button, gpointer data)
{
	SonatinaPathBar *pathbar = SONATINA_PATH_BAR(data);
	SonatinaPathBarPrivate *priv = sonatina_path_bar_get_instance_private(pathbar);
	GList *children;
	GList *cur;
	gint pos;
	gint i;

	gtk_container_child_get(GTK_CONTAINER(pathbar), GTK_WIDGET(button), "position", &pos, NULL);

	if (gtk_toggle_button_get_active(button)) {
		if (priv->selected != pos) {
			priv->selected = pos;
			g_signal_emit(G_OBJECT(data), path_bar_signals[CHANGED], 0, priv->selected);
		}
		children = gtk_container_get_children(GTK_CONTAINER(pathbar));
		for (cur = children, i = 0; cur; cur = cur->next, i++) {
			if (i != pos) {
				if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cur->data))) {
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cur->data), FALSE);
				}
			}
		}
		g_list_free(children);
	} else {
		if (pos == priv->selected) {
			gtk_toggle_button_set_active(button, TRUE);
		}
	}
}
