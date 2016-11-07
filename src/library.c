#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "library.h"
#include "client.h"
#include "util.h"
#include "songattr.h"
#include "core.h"
#include "gui.h"

const char *listing_icons[] = {
	[LIBRARY_FS] = "folder",
	[LIBRARY_GENRE] = "",
	[LIBRARY_ARTIST] = "",
	[LIBRARY_ALBUM] = "media-optical",
	[LIBRARY_SONG] = "audio-x-generic"
};

const char *listing_labels[] = {
	[LIBRARY_FS] = "Filesystem",
	[LIBRARY_GENRE] = "Genre",
	[LIBRARY_ARTIST] = "Artist",
	[LIBRARY_ALBUM] = "Album",
	[LIBRARY_SONG] = "Song"
};

gboolean library_tab_init(struct sonatina_tab *tab)
{
	struct library_tab *libtab = (struct library_tab *) tab;
	GObject *tw;
	GObject *header;
	GObject *selector;

	libtab->ui = load_tab_ui(tab->name);
	if (!libtab->ui) {
		return FALSE;
	}

	libtab->root = NULL;
	libtab->path = NULL;

	libtab->store = gtk_list_store_new(LIB_COL_COUNT, G_TYPE_STRING, G_TYPE_ICON);
	tw = gtk_builder_get_object(libtab->ui, "tw");
	library_tw_set_columns(GTK_TREE_VIEW(tw));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tw), GTK_TREE_MODEL(libtab->store));
	g_signal_connect(G_OBJECT(tw), "row-activated", G_CALLBACK(library_clicked_cb), libtab);

	header = gtk_builder_get_object(libtab->ui, "header");
	libtab->pathbar = sonatina_path_bar_new();
	g_signal_connect(G_OBJECT(libtab->pathbar), "changed", G_CALLBACK(library_pathbar_changed), libtab);
	gtk_box_pack_start(GTK_BOX(header), GTK_WIDGET(libtab->pathbar), FALSE, FALSE, 0);

	selector = gtk_builder_get_object(libtab->ui, "selector");
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(selector), library_selector_menu(libtab));
	library_set_listing(libtab, LIBRARY_ARTIST);

	connect_popup(GTK_WIDGET(tw), NULL);

	/* set tab widget */
	tab->widget = GTK_WIDGET(gtk_builder_get_object(libtab->ui, "top"));

	return TRUE;
}

void library_tw_set_columns(GtkTreeView *tw)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Icon", renderer, "gicon", LIB_COL_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tw), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", LIB_COL_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tw), column);
}

void library_tab_set_source(struct sonatina_tab *tab, GSource *source)
{
	struct library_tab *libtab = (struct library_tab *) tab;
	GObject *selector;

	selector = gtk_builder_get_object(libtab->ui, "selector");
	if (source) {
		libtab->mpdsource = source;
		mpd_source_register(source, MPD_CMD_LIST, library_list_cb, tab);
		mpd_source_register(source, MPD_CMD_LSINFO, library_lsinfo_cb, tab);
		mpd_source_register(source, MPD_CMD_FIND, library_lsinfo_cb, tab);
		library_load(libtab);
		gtk_widget_set_sensitive(GTK_WIDGET(selector), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(libtab->pathbar), TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(selector), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(libtab->pathbar), FALSE);
		gtk_list_store_clear(libtab->store);
	}
}

void library_list_cb(GList *args, union mpd_cmd_answer *answer, void *data)
{
	struct library_tab *tab = (struct library_tab *) data;
	GList *cur;
	GObject *tw;
	enum listing_type type;

	if (!args) {
		MSG_WARNING("irrelevant list answer received");
		return;
	}

	if (!g_strcmp0(args->data, "genre")) {
		type = LIBRARY_GENRE;
	} else if (!g_strcmp0(args->data, "artist")) {
		type = LIBRARY_ARTIST;
	} else if (!g_strcmp0(args->data, "album")) {
		type = LIBRARY_ALBUM;
	} else {
		MSG_WARNING("invalid list answer received: %s", args->data);
		type = LIBRARY_FS;
	}

	gtk_list_store_clear(tab->store);

	for (cur = answer->list; cur; cur = cur->next) {
		library_model_append(tab->store, type, cur->data);
	}

	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_widget_set_sensitive(GTK_WIDGET(tw), TRUE);
}

void library_lsinfo_cb(GList *args, union mpd_cmd_answer *answer, void *data)
{
	struct library_tab *tab = (struct library_tab *) data;
	const char *uri_arg;
	gchar *uri;
	GList *cur;
	GObject *tw;

	if (!tab->root) {
		return;
	}

	if (!tab->path) {
		return;
	}

	if (tab->root->type == LIBRARY_FS) {
		uri_arg = g_list_nth_data(args, 0);
		if (!uri_arg) {
			uri_arg = "";
		}
        
		uri = library_path_get_uri(tab->root, tab->path);
        
		MSG_DEBUG("received uri: %s; opened uri: %s", uri_arg, uri);
		if (g_strcmp0(uri, uri_arg)) {
			MSG_WARNING("irrelevant lsinfo answer received");
			g_free(uri);
			return;
		}
		g_free(uri);
	} else if (tab->path->type == LIBRARY_SONG) {
		/* TODO: check if songs are from relevant album? */
	} else {
		MSG_WARNING("irrelevant lsinfo answer received");
		return;
	}

	gtk_list_store_clear(tab->store);

	for (cur = answer->lsinfo.list; cur; cur = cur->next) {
		library_model_append_entity(tab->store, cur->data);
	}

	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_widget_set_sensitive(GTK_WIDGET(tw), TRUE);

}

void library_pathbar_changed(SonatinaPathBar *pathbar, gint selected, gpointer data)
{
	MSG_DEBUG("pathbar changed");
	library_tab_open_pos((struct library_tab *) data, selected);
}

void library_set_listing(struct library_tab *tab, enum listing_type listing)
{
	const char *title;

	library_path_free_all(tab->root);

	tab->root = library_path_root(listing);
	tab->path = tab->root;

	if (listing == LIBRARY_FS) {
		title = "Filesystem";
	} else {
		title = tab->root->name;
	}
	sonatina_path_bar_open_root(tab->pathbar, title, listing_icons[listing]);
}

void library_model_append_entity(GtkListStore *model, const struct mpd_entity *entity)
{
	const struct mpd_directory *dir;
	const struct mpd_song *song;
	gchar *name;
	gchar *format;
	enum listing_type type;

	switch (mpd_entity_get_type(entity)) {
	case MPD_ENTITY_TYPE_DIRECTORY:
		type = LIBRARY_FS;
		dir = mpd_entity_get_directory(entity);
		name = g_path_get_basename(mpd_directory_get_path(dir));
		break;
	case MPD_ENTITY_TYPE_SONG:
		type = LIBRARY_SONG;
		song = mpd_entity_get_song(entity);
		format = g_key_file_get_string(sonatina.rc, "library", "format", NULL);
		name = song_attr_format(format, song);
		MSG_DEBUG("library_model_append_entity(): format '%s', name '%s'", format, name);
		g_free(format);
		break;
	default:
		name = g_strdup("unknown");
		break;
	}

	library_model_append(model, type, name);
	g_free(name);
}

void library_model_append(GtkListStore *model, enum listing_type type, const char *name)
{
	GtkTreeIter iter;
	GIcon *icon;

	icon = g_icon_new_for_string(listing_icons[type], NULL);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter, LIB_COL_ICON, icon, LIB_COL_NAME, name, -1);
}

void library_tab_destroy(struct sonatina_tab *tab)
{
	struct library_tab *libtab = (struct library_tab *) tab;
	struct library_path *path;

	g_object_unref(G_OBJECT(libtab->ui));

	for (path = libtab->root; path; path = path->next) {
		library_path_free(path);
	}
}

struct library_path *library_path_root(enum listing_type listing)
{
	struct library_path *path;

	path = g_malloc(sizeof(struct library_path));

	path->type = listing;
	path->parent = NULL;
	path->next = NULL;
	path->name = NULL;
	path->selected = -1;

	switch (listing) {
	case LIBRARY_FS:
		path->name = g_strdup("");
		break;
	case LIBRARY_GENRE:
		path->name = g_strdup("Genre");
		break;
	case LIBRARY_ARTIST:
		path->name = g_strdup("Artist");
		break;
	case LIBRARY_ALBUM:
		path->name = g_strdup("Album");
		break;
	default:
		break;
	}

	return path;
}

void library_path_free(struct library_path *path)
{
	if (!path) {
		return;
	}

	if (path->name) {
		g_free(path->name);
	}

	g_free(path);
}

void library_path_free_all(struct library_path *root)
{
	struct library_path *path;

	if (!root) {
		return;
	}

	for (path = root->next; path; path = path->next) {
		library_path_free(path->parent);
	}
}

void library_load(struct library_tab *tab)
{
	gchar *uri;

	switch (tab->path->type) {
	case LIBRARY_FS:
		uri = library_path_get_uri(tab->root, tab->path);
		MSG_DEBUG("sending lsinfo %s", uri);
		mpd_send(tab->mpdsource, MPD_CMD_LSINFO, uri, NULL);
		g_free(uri);
		break;
	case LIBRARY_GENRE:
		MSG_INFO("opening genre list");
		mpd_send(tab->mpdsource, MPD_CMD_LIST, "genre", NULL);
		break;
	case LIBRARY_ARTIST:
		MSG_INFO("opening artist list");
		if (tab->path->parent && tab->path->parent->type == LIBRARY_GENRE) {
			mpd_send(tab->mpdsource, MPD_CMD_LIST, "artist",
					"genre", tab->path->name, NULL);
		} else {
			mpd_send(tab->mpdsource, MPD_CMD_LIST, "artist", NULL);
		}
		break;
	case LIBRARY_ALBUM:
		MSG_INFO("opening album list");
		if (tab->path->parent && tab->path->parent->type == LIBRARY_ARTIST) {
			if (tab->path->parent && tab->path->parent->type == LIBRARY_GENRE) {
				mpd_send(tab->mpdsource, MPD_CMD_LIST, "album",
						"artist", tab->path->name,
						"genre", tab->path->parent->name, NULL);
			} else {
				MSG_DEBUG("listing albums for artist %s", tab->path->name);
				mpd_send(tab->mpdsource, MPD_CMD_LIST, "album",
						"artist", tab->path->name, NULL);
			}
		} else {
			mpd_send(tab->mpdsource, MPD_CMD_LIST, "album", NULL);
		}
		break;
	case LIBRARY_SONG:
		MSG_INFO("opening song list");
		mpd_send(tab->mpdsource, MPD_CMD_FIND, "album", tab->path->name, NULL);
		break;
	default:
		break;
	}
}

void library_tab_open_dir(struct library_tab *tab, const char *name)
{
	struct library_path *path;
	GObject *tw;
	const char *icon;

	path = library_path_open(tab->path, name);
	if (!path) {
		return;
	}
	tab->path = path;
	if (path->parent) {
		icon = listing_icons[path->parent->type];
	} else {
		icon = listing_icons[path->type];
	}
	sonatina_path_bar_open(tab->pathbar, name, icon);
	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_widget_set_sensitive(GTK_WIDGET(tw), FALSE);
	library_load(tab);
}

void library_tab_open_pos(struct library_tab *tab, int pos)
{
	struct library_path *path;
	int i;
	GObject *tw;

	for (path = tab->root, i = 0; path; path = path->next, i++) {
		if (i == pos) break;
	}

	if (!path) {
		return;
	}
	tab->path = path;
	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_widget_set_sensitive(GTK_WIDGET(tw), FALSE);
	library_load(tab);
}

void library_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, struct library_tab *tab)
{
	GtkTreeIter iter;
	gchar *name;

	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(tab->store), &iter, path)) {
		MSG_WARNING("library dir item vanished");
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(tab->store), &iter, LIB_COL_NAME, &name, -1);
	library_tab_open_dir(tab, name);
}

struct library_path *library_path_open(struct library_path *parent, const char *name)
{
	struct library_path *path;
	enum listing_type type;

	g_assert(parent != NULL);
	g_assert(name != NULL);

	MSG_DEBUG("library_path_open(): %s", name);

	if (parent->type == LIBRARY_FS) {
		type = LIBRARY_FS;
	} else {
		type = parent->type + 1;
	}

	if (type > LIBRARY_SONG) {
		return NULL;
	}

	path = g_malloc(sizeof(struct library_path));
	path->type = type;
	path->parent = parent;
	path->next = NULL;
	path->name = g_strdup(name);
	path->selected = -1;

	if (parent->next) {
		library_path_free(parent->next);
	}

	parent->next = path;

	return path;
}

void library_path_close(struct library_path *path)
{
	path->parent->next = NULL;
	library_path_free(path);
}

gchar *library_path_get_uri(const struct library_path *root, const struct library_path *path)
{
	GString *uri;
	const struct library_path *cur;

	if (root->type != LIBRARY_FS) {
		return NULL;
	}

	uri = g_string_new(NULL);
	for (cur = root; cur && cur != path->next; cur = cur->next) {
		if (strlen(uri->str) > 0) {
			g_string_append(uri, "/");
		}
		g_string_append(uri, cur->name);
	}

	return g_string_free(uri, FALSE);
}

void library_selector_activate(GtkMenuItem *item, gpointer data)
{
	struct library_tab *tab = (struct library_tab *) data;
	gint type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "listing"));

	library_set_listing(tab, type);
	library_load(tab);
}

GtkWidget *library_selector_menu(struct library_tab *tab)
{
	GtkWidget *menu;
	GtkWidget *item;
	int i;

	menu = gtk_menu_new();

	for (i = 0; i < LIBRARY_SONG; i++) {
		item = gtk_menu_item_new_with_label(listing_labels[i]);
		g_object_set_data(G_OBJECT(item), "listing", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(library_selector_activate), tab);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	gtk_widget_show_all(menu);

	return menu;
}

