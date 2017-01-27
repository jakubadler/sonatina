#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <mpd/client.h>

#include "library.h"
#include "client.h"
#include "util.h"
#include "songattr.h"
#include "core.h"
#include "gui.h"
#include "settings.h"
#include "gettext.h"

const char *listing_icons[] = {
	[LIBRARY_PLAYLISTSONG] = "audio-x-generic",
	[LIBRARY_PLAYLIST] = "folder-music",
	[LIBRARY_FS] = "folder",
	[LIBRARY_GENRE] = "user-bookmarks",
	[LIBRARY_ARTIST] = "avatar-default",
	[LIBRARY_ALBUM] = "media-optical",
	[LIBRARY_SONG] = "audio-x-generic"
};

const char *listing_labels(enum listing_type type)
{
	switch (type) {
	case LIBRARY_FS:
		return _("Filesystem");
	case LIBRARY_PLAYLIST:
		return _("Playlists");
	case LIBRARY_GENRE:
		return _("Genre");
	case LIBRARY_ARTIST:
		return _("Artist");
	case LIBRARY_ALBUM:
		return _("Album");
	case LIBRARY_SONG:
		return _("Song");
	default:
		break;
	}

	return NULL;
}

static GActionEntry library_selected_actions[] = {
	{ "add", library_add_action, NULL, NULL, NULL },
	{ "replace", library_replace_action, NULL, NULL, NULL }
};

static GActionEntry library_selected_fs_actions[] = {
	{ "update", library_update_action, NULL, NULL, NULL }
};

static GActionEntry library_selected_pl_actions[] = {
	{ "delete", library_delete_action, NULL, NULL, NULL }
};

gboolean library_tab_init(struct sonatina_tab *tab)
{
	struct library_tab *libtab = (struct library_tab *) tab;
	GObject *tw;
	GObject *header;
	GObject *selector;
	GObject *menu;
	GtkTreeSelection *selection;

	libtab->ui = load_tab_ui(tab->name);
	if (!libtab->ui) {
		return FALSE;
	}

	libtab->root = NULL;
	libtab->path = NULL;

	libtab->store = gtk_list_store_new(LIB_COL_COUNT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ICON, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(libtab->store), LIB_COL_DISPLAY_NAME, GTK_SORT_ASCENDING);

	tw = gtk_builder_get_object(libtab->ui, "tw");
	library_tw_set_columns(GTK_TREE_VIEW(tw));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tw), GTK_TREE_MODEL(libtab->store));
	g_signal_connect(G_OBJECT(tw), "row-activated", G_CALLBACK(library_clicked_cb), libtab);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tw));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(library_selection_changed), libtab);

	header = gtk_builder_get_object(libtab->ui, "header");
	libtab->pathbar = sonatina_path_bar_new();
	g_signal_connect(G_OBJECT(libtab->pathbar), "changed", G_CALLBACK(library_pathbar_changed), libtab);
	gtk_box_pack_start(GTK_BOX(header), GTK_WIDGET(libtab->pathbar), FALSE, FALSE, 0);

	selector = gtk_builder_get_object(libtab->ui, "selector");
	gtk_menu_button_set_popup(GTK_MENU_BUTTON(selector), library_selector_menu(libtab));
	library_set_listing(libtab, LIBRARY_ARTIST);

	/* set tab widget */
	tab->widget = GTK_WIDGET(gtk_builder_get_object(libtab->ui, "top"));

	menu = gtk_builder_get_object(libtab->ui, "menu");
	connect_popup(GTK_WIDGET(tw), G_MENU_MODEL(menu));
	connect_popup(GTK_WIDGET(tab->widget), G_MENU_MODEL(menu));

	return TRUE;
}

void library_tw_set_columns(GtkTreeView *tw)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(G_OBJECT(renderer), "stock-size", sonatina_settings_get_num("library", "icon_size"), NULL);
	column = gtk_tree_view_column_new_with_attributes("Icon", renderer, "gicon", LIB_COL_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tw), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", LIB_COL_DISPLAY_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tw), column);
}

void library_tab_set_source(struct sonatina_tab *tab, GSource *source)
{
	struct library_tab *libtab = (struct library_tab *) tab;
	GObject *selector;

	selector = gtk_builder_get_object(libtab->ui, "selector");

	libtab->mpdsource = source;
	if (source) {
		mpd_source_register(source, MPD_CMD_LIST, library_list_cb, tab);
		mpd_source_register(source, MPD_CMD_LSINFO, library_lsinfo_cb, tab);
		mpd_source_register(source, MPD_CMD_FIND, library_lsinfo_cb, tab);
		mpd_source_register(source, MPD_CMD_LISTPLINFO, library_lsinfo_cb, tab);
		mpd_source_register(source, MPD_CMD_LISTPLS, library_lsinfo_cb, tab);
		mpd_source_register(source, MPD_CMD_IDLE, library_idle_cb, tab);
		library_load(libtab);
		gtk_widget_set_sensitive(GTK_WIDGET(selector), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(libtab->pathbar), TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(selector), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(libtab->pathbar), FALSE);
		gtk_list_store_clear(libtab->store);
	}
}

void library_list_cb(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	struct library_tab *tab = (struct library_tab *) data;
	GList *cur;
	enum listing_type type;
	GtkTreeIter iter;
	const gchar *name;

	if (!args) {
		MSG_WARNING("irrelevant list answer received");
		return;
	}

	if (!g_strcmp0(args->data, "genre")) {
		type = LIBRARY_GENRE;
	} else if (!g_strcmp0(args->data, "artist") || !g_strcmp0(args->data, "albumartist")) {
		type = LIBRARY_ARTIST;
	} else if (!g_strcmp0(args->data, "album")) {
		type = LIBRARY_ALBUM;
	} else {
		MSG_WARNING("invalid list answer received: %s", args->data);
		type = LIBRARY_FS;
	}

	gtk_list_store_clear(tab->store);

	for (cur = answer->list.list; cur; cur = cur->next) {
		switch (type) {
		case LIBRARY_GENRE:
			name = ((struct mpd_tag_entity *) cur->data)->genre;
			break;
		case LIBRARY_ARTIST:
			name = ((struct mpd_tag_entity *) cur->data)->artist;
			break;
		case LIBRARY_ALBUM:
			name = ((struct mpd_tag_entity *) cur->data)->album;
			break;
		default:
			name = NULL;
			break;
		}
		iter = library_model_append(tab->store, type, name, NULL, MPD_ENTITY_TYPE_UNKNOWN);
		if (!g_strcmp0(tab->path->selected, name)) {
			library_select(tab, &iter);
		}
	}

	library_tab_set_scroll(tab);
	library_set_busy(tab, FALSE);
}

void library_lsinfo_cb(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	struct library_tab *tab = (struct library_tab *) data;
	const char *uri_arg;
	gchar *uri;
	GList *cur;
	GtkTreeIter iter;

	if (!tab->root) {
		return;
	}

	if (!tab->path) {
		return;
	}

	if (tab->path->type == LIBRARY_FS) {
		if (cmd != MPD_CMD_LSINFO) {
			MSG_WARNING("irrelevant answer received for LIBRARY_FS list");
			return;
		}
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
		if (cmd != MPD_CMD_FIND) {
			MSG_WARNING("irrelevant answer received for LIBRARY_SONG list");
			return;
		}
		if (g_strcmp0(g_list_nth_data(args, 1), tab->path->name)) {
			MSG_WARNING("received answer for irrelevant album");
			return;
		}
	} else if (tab->path->type == LIBRARY_PLAYLIST) {
		if (cmd != MPD_CMD_LISTPLS) {
			MSG_WARNING("irrelevant answer received while expecting listplaylists");
			return;
		}
	} else if (tab->path->type == LIBRARY_PLAYLISTSONG) {
		if (cmd != MPD_CMD_LISTPL && cmd != MPD_CMD_LISTPLINFO) {
			MSG_WARNING("irrelevant answer received while expecting listplaylist");
			return;
		}
		uri_arg = g_list_nth_data(args, 0);
		uri = library_path_get_uri(tab->root, tab->path);
		if (g_strcmp0(uri_arg, uri)) {
			MSG_WARNING("irrelevant playlistinfo answer received");
			g_free(uri);
			return;
		}
		g_free(uri);
	} else {
		MSG_WARNING("irrelevant lsinfo answer received");
		return;
	}

	gtk_list_store_clear(tab->store);
	
	for (cur = answer->lsinfo.list; cur; cur = cur->next) {
		iter = library_model_append_entity(tab->store, cur->data);
		if (!g_strcmp0(tab->path->selected, cur->data)) {
			library_select(tab, &iter);
		}
	}

	library_tab_set_scroll(tab);
	library_set_busy(tab, FALSE);
}

void library_idle_cb(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	struct library_tab *tab = (struct library_tab *) data;

	if (answer->idle & MPD_CHANGED_DB)
	{
		MSG_INFO("library changed");
		library_load(tab);
	}
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
		title = _("Filesystem");
	} else if (listing == LIBRARY_PLAYLIST) {
		title = _("Playlists");
	} else {
		title = tab->root->name;
	}
	sonatina_path_bar_open_root(tab->pathbar, title, listing_icons[listing]);
}

GtkTreeIter library_model_append_entity(GtkListStore *model, const struct mpd_entity *entity)
{
	const struct mpd_directory *dir;
	const struct mpd_song *song;
	const struct mpd_playlist *pl;
	gchar *name;
	gchar *format;
	const char *uri = NULL;
	enum mpd_entity_type mpdtype;
	enum listing_type type;
	GtkTreeIter iter;

	mpdtype = mpd_entity_get_type(entity);

	switch (mpdtype) {
	case MPD_ENTITY_TYPE_DIRECTORY:
		type = LIBRARY_FS;
		dir = mpd_entity_get_directory(entity);
		uri = mpd_directory_get_path(dir);
		name = g_path_get_basename(uri);
		break;
	case MPD_ENTITY_TYPE_SONG:
		type = LIBRARY_SONG;
		song = mpd_entity_get_song(entity);
		format = sonatina_settings_get_string("library", "format");
		name = song_attr_format(format, song);
		uri = mpd_song_get_uri(song);
		MSG_DEBUG("library_model_append_entity(): format '%s', name '%s', uri: %s", format, name, uri);
		g_free(format);
		break;
	case MPD_ENTITY_TYPE_PLAYLIST:
		type = LIBRARY_PLAYLIST;
		pl = mpd_entity_get_playlist(entity);
		uri = mpd_playlist_get_path(pl);
		name = g_path_get_basename(uri);
		break;
	default:
		type = LIBRARY_FS;
		name = g_strdup(_("unknown"));
		break;
	}

	iter = library_model_append(model, type, name, uri, mpdtype);
	g_free(name);

	return iter;
}

GtkTreeIter library_model_append(GtkListStore *model, enum listing_type type, const char *name, const char *uri, enum mpd_entity_type mpdtype)
{
	GtkTreeIter iter;
	GIcon *icon;
	const char *display;

	if (!name || strlen(name) == 0) {
		switch (type) {
		case LIBRARY_GENRE:
			display = _("Unknown genre");
			break;
		case LIBRARY_ARTIST:
			display = _("Unknown artist");
			break;
		case LIBRARY_ALBUM:
			display = _("Unknown album");
			break;
		default:
			display = _("Unknown");
			break;
		}
	} else {
		display = name;
		name = NULL;
	}

	icon = g_icon_new_for_string(listing_icons[type], NULL);
	gtk_list_store_append(model, &iter);
	gtk_list_store_set(model, &iter,
			LIB_COL_ICON, icon,
			LIB_COL_DISPLAY_NAME, display,
			LIB_COL_NAME, name,
			LIB_COL_URI, uri,
			LIB_COL_TYPE, mpdtype, -1);
	g_object_unref(icon);

	return iter;
}

void library_tab_destroy(struct sonatina_tab *tab)
{
	struct library_tab *libtab = (struct library_tab *) tab;
	struct library_path *path;

	gtk_widget_destroy(tab->widget);

	g_object_unref(libtab->ui);
	g_object_unref(libtab->store);
	g_object_unref(libtab->pathbar);

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
	path->selected = NULL;
	path->pos = NULL;

	switch (listing) {
	case LIBRARY_FS:
		path->name = g_strdup("");
		break;
	case LIBRARY_PLAYLIST:
		path->name = g_strdup(_("Playlists"));
		break;
	case LIBRARY_GENRE:
		path->name = g_strdup(_("Genre"));
		break;
	case LIBRARY_ARTIST:
		path->name = g_strdup(_("Artist"));
		break;
	case LIBRARY_ALBUM:
		path->name = g_strdup(_("Album"));
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

	if (path->selected) {
		g_free(path->selected);
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

gboolean library_load(struct library_tab *tab)
{
	gchar *uri;
	gboolean retval = FALSE;

	library_set_busy(tab, TRUE);

	switch (tab->path->type) {
	case LIBRARY_FS:
		uri = library_path_get_uri(tab->root, tab->path);
		MSG_DEBUG("sending lsinfo %s", uri);
		retval = mpd_send(tab->mpdsource, MPD_CMD_LSINFO, uri, NULL);
		g_free(uri);
		break;
	case LIBRARY_PLAYLIST:
		MSG_INFO("opening stored playlists list");
		retval = mpd_send(tab->mpdsource, MPD_CMD_LISTPLS, NULL);
		break;
	case LIBRARY_PLAYLISTSONG:
		uri = library_path_get_uri(tab->root, tab->path);
		MSG_INFO("opening playlist '%s'", uri);
		retval = mpd_send(tab->mpdsource, MPD_CMD_LISTPLINFO, uri, NULL);
		g_free(uri);
		break;
	case LIBRARY_GENRE:
		MSG_INFO("opening genre list");
		retval = mpd_send(tab->mpdsource, MPD_CMD_LIST, "genre", NULL);
		break;
	case LIBRARY_ARTIST:
		MSG_INFO("opening artist list");
		if (tab->path->parent && tab->path->parent->type == LIBRARY_GENRE) {
			retval = mpd_send(tab->mpdsource, MPD_CMD_LIST, "albumartist",
					"genre", tab->path->name, NULL);
		} else {
			retval = mpd_send(tab->mpdsource, MPD_CMD_LIST, "albumartist", NULL);
		}
		break;
	case LIBRARY_ALBUM:
		MSG_INFO("opening album list");
		if (tab->path->parent && tab->path->parent->type == LIBRARY_ARTIST) {
			if (tab->path->parent && tab->path->parent->type == LIBRARY_GENRE) {
				retval = mpd_send(tab->mpdsource, MPD_CMD_LIST, "album",
						"albumartist", tab->path->name,
						"genre", tab->path->parent->name, NULL);
			} else {
				MSG_DEBUG("listing albums for artist %s", tab->path->name);
				retval = mpd_send(tab->mpdsource, MPD_CMD_LIST, "album",
						"albumartist", tab->path->name, NULL);
			}
		} else {
			retval = mpd_send(tab->mpdsource, MPD_CMD_LIST, "album", NULL);
		}
		break;
	case LIBRARY_SONG:
		MSG_INFO("opening song list");
		retval = mpd_send(tab->mpdsource, MPD_CMD_FIND, "album", tab->path->name, NULL);
		break;
	default:
		retval = FALSE;
		break;
	}

	return retval;
}

gboolean library_add(struct library_tab *tab, GtkTreeIter iter)
{
	gchar *display_name;
	gchar *name;
	gchar *uri;
	enum mpd_entity_type type;
	gboolean retval = FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(tab->store), &iter,
			LIB_COL_DISPLAY_NAME, &display_name,
			LIB_COL_NAME, &name,
			LIB_COL_URI, &uri,
			LIB_COL_TYPE, &type, -1);

	if (!name) {
		name = display_name;
		display_name = NULL;
	}

	switch (tab->path->type) {
	case LIBRARY_FS:
		MSG_DEBUG("sending add %s", uri);
		if (type == MPD_ENTITY_TYPE_PLAYLIST) {
			retval = mpd_send(tab->mpdsource, MPD_CMD_LOAD, uri, NULL);
		} else {
			retval = mpd_send(tab->mpdsource, MPD_CMD_ADD, uri, NULL);
		}
		break;
	case LIBRARY_PLAYLIST:
		MSG_INFO("adding playlist %s", name);
		retval = mpd_send(tab->mpdsource, MPD_CMD_LOAD, name, NULL);
		break;
	case LIBRARY_GENRE:
		MSG_INFO("adding genre %s", name);
		retval = mpd_send(tab->mpdsource, MPD_CMD_FINDADD, "genre", name, NULL);
		break;
	case LIBRARY_ARTIST:
		MSG_INFO("adding artist %s", name);
		if (tab->path->parent && tab->path->parent->type == LIBRARY_GENRE) {
			retval = mpd_send(tab->mpdsource, MPD_CMD_FINDADD, "albumartist", name,
					"genre", tab->path->name, NULL);
		} else {
			retval = mpd_send(tab->mpdsource, MPD_CMD_FINDADD, "albumartist", name, NULL);
		}
		break;
	case LIBRARY_ALBUM:
		MSG_INFO("adding album %s", name);
		if (tab->path->parent && tab->path->parent->type == LIBRARY_ARTIST) {
			if (tab->path->parent && tab->path->parent->type == LIBRARY_GENRE) {
				retval = mpd_send(tab->mpdsource, MPD_CMD_FINDADD, "album", name,
						"albumartist", tab->path->name,
						"genre", tab->path->parent->name, NULL);
			} else {
				MSG_DEBUG("adding album %s from artist %s", name, tab->path->name);
				retval = mpd_send(tab->mpdsource, MPD_CMD_FINDADD, "album", name,
						"albumartist", tab->path->name, NULL);
			}
		} else {
			retval = mpd_send(tab->mpdsource, MPD_CMD_FINDADD, "album", name, NULL);
		}
		break;
	case LIBRARY_SONG:
	case LIBRARY_PLAYLISTSONG:
		MSG_INFO("adding song %s", name);
		retval = mpd_send(tab->mpdsource, MPD_CMD_ADD, uri, NULL);
		break;
	default:
		retval = FALSE;
		break;
	}

	g_free(name);
	g_free(display_name);
	g_free(uri);

	return retval;
}

void library_tab_save_scroll(struct library_tab *tab)
{
	GObject *tw;

	g_assert(tab != NULL);
	g_assert(tab->path != NULL);

	tw = gtk_builder_get_object(tab->ui, "tw");

	if (tab->path->pos) {
		gtk_tree_path_free(tab->path->pos);
	}
	gtk_tree_view_get_visible_range(GTK_TREE_VIEW(tw), &tab->path->pos, NULL);
}

void library_tab_set_scroll(struct library_tab *tab)
{
	GObject *tw;

	g_assert(tab != NULL);
	g_assert(tab->path != NULL);

	if (!tab->path->pos) {
		return;
	}

	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tw), tab->path->pos, NULL, TRUE, 0.0, 0.0);
}

void library_tab_open_dir(struct library_tab *tab, GtkTreeIter iter)
{
	struct library_path *path;
	const char *icon;
	gchar *name;
	gchar *display_name;
	enum mpd_entity_type type;

	library_tab_save_scroll(tab);

	gtk_tree_model_get(GTK_TREE_MODEL(tab->store), &iter,
			LIB_COL_NAME, &name,
			LIB_COL_DISPLAY_NAME, &display_name,
			LIB_COL_TYPE, &type, -1);

	if (!name) {
		name = g_strdup(display_name);
	}

	if (tab->path->selected) {
		g_free(tab->path->selected);
	}

	tab->path->selected = name;

	path = library_path_open(tab->path, name, type);

	if (!path) {
		return;
	}

	tab->path = path;

	if (path->parent) {
		icon = listing_icons[path->parent->type];
	} else {
		icon = listing_icons[path->type];
	}

	sonatina_path_bar_open(tab->pathbar, display_name, icon);
	library_load(tab);

	g_free(display_name);
}

void library_tab_open_pos(struct library_tab *tab, int pos)
{
	struct library_path *path;
	int i;

	library_tab_save_scroll(tab);

	for (path = tab->root, i = 0; path; path = path->next, i++) {
		if (i == pos) break;
	}

	if (!path) {
		return;
	}
	tab->path = path;
	library_load(tab);
}

void library_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, struct library_tab *tab)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(tab->store), &iter, path)) {
		MSG_WARNING("library dir item vanished");
		return;
	}

	library_tab_open_dir(tab, iter);
}

struct library_path *library_path_open(struct library_path *parent, const char *name, enum mpd_entity_type mpdtype)
{
	struct library_path *path;
	enum listing_type type;

	g_assert(parent != NULL);
	g_assert(name != NULL);

	MSG_DEBUG("library_path_open(): %s", name);

	switch (parent->type) {
	case LIBRARY_FS:
		if (mpdtype == MPD_ENTITY_TYPE_DIRECTORY) {
			type = LIBRARY_FS;
		} else if (mpdtype == MPD_ENTITY_TYPE_PLAYLIST) {
			type = LIBRARY_PLAYLISTSONG;
		} else {
			return NULL;
		}
		break;
	case LIBRARY_PLAYLIST:
		type = LIBRARY_PLAYLISTSONG;
		break;
	case LIBRARY_SONG:
	case LIBRARY_PLAYLISTSONG:
		/* TODO: Add item to playlist instead */
		return NULL;
		break;
	default:
		type = parent->type + 1;
	}

	path = g_malloc(sizeof(struct library_path));
	path->type = type;
	path->parent = parent;
	path->next = NULL;
	path->name = g_strdup(name);
	path->selected = NULL;
	path->pos = NULL;

	if (!path) {
		return NULL;
	}

	if (parent->next) {
		library_path_free_all(parent->next);
	}

	parent->next = path;

	return path;
}

gchar *library_path_get_uri(const struct library_path *root, const struct library_path *path)
{
	GString *uri;
	const struct library_path *cur;

	if (root->type > LIBRARY_GENRE) {
		return NULL;
	}

	uri = g_string_new(NULL);
	for (cur = root; cur && cur != path->next; cur = cur->next) {
		if (cur->type == LIBRARY_PLAYLIST) {
			continue;
		}
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

	for (i = LIBRARY_PLAYLIST; i < LIBRARY_SONG; i++) {
		item = gtk_menu_item_new_with_label(listing_labels(i));
		g_object_set_data(G_OBJECT(item), "listing", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(library_selector_activate), tab);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	gtk_widget_show_all(menu);

	return menu;
}

gboolean library_process_selected(struct library_tab *tab, gboolean (*row_func)(struct library_tab *, GtkTreeIter))
{
	GObject *tw;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *rows;
	GList *cur;
	gboolean retval = TRUE;

	tw = gtk_builder_get_object(tab->ui, "tw");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tw));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (cur = rows; cur; cur = cur->next) {
		if (!gtk_tree_model_get_iter(model, &iter, cur->data)) {
			continue;
		}
		row_func(tab, iter);
	}

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);

	return retval;
}

void library_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	struct library_tab *tab = (struct library_tab *) data;
	GtkTreeModel *model;
	GList *rows;
	GtkTreeView *tw;
	GSimpleActionGroup *actions;

	tw = gtk_tree_selection_get_tree_view(selection);
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (rows) {
		actions = g_simple_action_group_new();
		g_action_map_add_action_entries(G_ACTION_MAP(actions), library_selected_actions, G_N_ELEMENTS(library_selected_actions), tab);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "library-selected", G_ACTION_GROUP(actions));
		g_object_unref(actions);
		if (tab->path->type == LIBRARY_FS) {
			actions = g_simple_action_group_new();
			g_action_map_add_action_entries(G_ACTION_MAP(actions), library_selected_fs_actions, G_N_ELEMENTS(library_selected_fs_actions), tab);
			gtk_widget_insert_action_group(GTK_WIDGET(tw), "library-selected-fs", G_ACTION_GROUP(actions));
			g_object_unref(actions);
		} else if (tab->path->type == LIBRARY_PLAYLIST) {
			actions = g_simple_action_group_new();
			g_action_map_add_action_entries(G_ACTION_MAP(actions), library_selected_pl_actions, G_N_ELEMENTS(library_selected_pl_actions), tab);
			gtk_widget_insert_action_group(GTK_WIDGET(tw), "library-selected-pl", G_ACTION_GROUP(actions));
			g_object_unref(actions);
		}
	} else {
		/* nothing selected */
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "library-selected", NULL);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "library-selected-fs", NULL);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "library-selected-pl", NULL);
	}

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

void library_add_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	MSG_INFO("Add action activated");

	library_process_selected((struct library_tab *) data, library_add);
}

void library_replace_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	struct library_tab *tab = (struct library_tab *) data;

	MSG_INFO("Replace action activated");

	mpd_send(tab->mpdsource, MPD_CMD_CLEAR, NULL);
	library_process_selected(tab, library_add);
}

void library_update_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	struct library_tab *tab = (struct library_tab *) data;
	GObject *tw;
	GtkTreeSelection *select;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *rows;
	GList *row;
	gchar *uri;

	g_assert(tab->path->type == LIBRARY_FS);

	tw = gtk_builder_get_object(tab->ui, "tw");
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tw));
	rows = gtk_tree_selection_get_selected_rows(select, &model);

	for (row = rows; row; row = row->next) {
		if (!gtk_tree_model_get_iter(model, &iter, row->data)) {
			continue;
		}
		gtk_tree_model_get(model, &iter, LIB_COL_URI, &uri, -1);
		if (uri) {
			mpd_send(tab->mpdsource, MPD_CMD_UPDATE, uri, NULL);
			g_free(uri);
		}
	}

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

gboolean library_delete_playlist(struct library_tab *tab, GtkTreeIter iter)
{
	gchar *uri;
	gboolean success;

	gtk_tree_model_get(GTK_TREE_MODEL(tab->store), &iter, LIB_COL_URI, &uri, -1);
	success = mpd_send(tab->mpdsource, MPD_CMD_RM, uri, NULL);
	g_free(uri);

	return success;
}

void library_delete_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	MSG_INFO("Delete action activated");

	library_process_selected(data, library_delete_playlist);
}

void library_set_busy(struct library_tab *tab, gboolean busy)
{
	GObject *spinner;
	GObject *tw;

	spinner = gtk_builder_get_object(tab->ui, "spinner");
	g_object_set(spinner, "active", busy, NULL);

	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_widget_set_sensitive(GTK_WIDGET(tw), !busy);
}

void library_select(struct library_tab *tab, GtkTreeIter *iter)
{
	GObject *tw;
	GtkTreeSelection *select;

	tw = gtk_builder_get_object(tab->ui, "tw");
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tw));

	gtk_tree_selection_select_iter(select, iter);
}

