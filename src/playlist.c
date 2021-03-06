#include <gtk/gtk.h>

#include "core.h"
#include "playlist.h"
#include "songattr.h"
#include "util.h"
#include "client.h"
#include "gui.h"
#include "settings.h"

static GActionEntry playlist_selected_actions[] = {
	{ "remove", playlist_remove_action, NULL, NULL, NULL }
};

static GActionEntry playlist_actions[] = {
	{ "clear", playlist_clear_action, NULL, NULL, NULL },
	{ "shuffle", playlist_shuffle_action, NULL, NULL, NULL },
};

gboolean pl_tab_init(struct sonatina_tab *tab)
{
	struct pl_tab *pltab = (struct pl_tab *) tab;
	GObject *tw;
	GObject *menu;
	GtkTreeSelection *selection;
	gchar *format;

	/* load ui */
	pltab->ui = load_tab_ui(tab->name);
	if (!pltab->ui) {
		MSG_ERROR("Failed to load ui file for tab %s", tab->name);
		return FALSE;
	}

	pltab->store = NULL;
	format = sonatina_settings_get_string("playlist", "format");
	pl_tab_set_format(pltab, format);
	g_free(format);

	/* update TreeView */
	tw = gtk_builder_get_object(pltab->ui, "tw");
	if (!tw) {
		MSG_INFO("failed to load playlist tree view");
		return FALSE;
	}
	g_object_set(G_OBJECT(tw), "reorderable", TRUE, NULL);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tw));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect(G_OBJECT(tw), "row-activated", G_CALLBACK(playlist_clicked_cb), NULL);
	g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(pl_selection_changed), pltab);

	/* set tab widget */
	tab->widget = GTK_WIDGET(gtk_builder_get_object(pltab->ui, "top"));

	pltab->mpdsource = NULL;

	menu = gtk_builder_get_object(pltab->ui, "menu");
	connect_popup(GTK_WIDGET(tw), G_MENU_MODEL(menu));
	connect_popup(GTK_WIDGET(tab->widget), G_MENU_MODEL(menu));

	return TRUE;
}

void pl_tab_set_format(struct pl_tab *tab, const char *format)
{
	size_t i;
	GType *types;
	GObject *tw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	gchar *title;

	tab->n_columns = 1;
	for (i = 0; format[i]; i++) {
		if (format[i] == '|')
			tab->n_columns++;
	}

	tab->columns = g_strsplit(format, "|", 0);

	types = g_malloc((PL_COUNT + tab->n_columns) * sizeof(GType));

	types[PL_ID] = G_TYPE_INT;
	types[PL_POS] = G_TYPE_INT;
	types[PL_WEIGHT] = G_TYPE_INT;
	for (i = 0; tab->columns[i]; i++) {
		types[PL_COUNT + i] = G_TYPE_STRING;
	}

	if (tab->store) {
		g_object_unref(G_OBJECT(tab->store));
	}

	tab->store = gtk_list_store_newv(PL_COUNT + tab->n_columns, types);
	g_signal_connect(G_OBJECT(tab->store), "row-changed", G_CALLBACK(playlist_row_changed_cb), tab);
	g_free(types);

	tw = gtk_builder_get_object(tab->ui, "tw");
	gtk_tree_view_set_model(GTK_TREE_VIEW(tw), GTK_TREE_MODEL(tab->store));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "weight-set", TRUE, NULL);

	/* remove all columns */
	for (i = 0; (col = gtk_tree_view_get_column(GTK_TREE_VIEW(tw), i)); i++) {
		gtk_tree_view_remove_column(GTK_TREE_VIEW(tw), col);
	}

	/* add new columns */
	for (i = 0; tab->columns[i]; i++) {
		title = song_attr_format(tab->columns[i], NULL);
		MSG_DEBUG("adding column with format '%s' to playlist tw", tab->columns[i]);
		col = gtk_tree_view_column_new_with_attributes(title, renderer, "weight", PL_WEIGHT, "text", PL_COUNT + i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tw), col);
	}
}

void pl_tab_set_source(struct sonatina_tab *tab, GSource *source)
{
	struct pl_tab *pltab = (struct pl_tab *) tab;
	GObject *tw;
	GSimpleActionGroup *actions;

	pltab->mpdsource = source;
	tw = gtk_builder_get_object(pltab->ui, "tw");

	if (source) {
		mpd_source_register(source, MPD_CMD_CURRENTSONG, pl_process_song, tab);
		mpd_source_register(source, MPD_CMD_PLINFO, pl_process_pl, tab);

		actions = g_simple_action_group_new();
		g_action_map_add_action_entries(G_ACTION_MAP(actions), playlist_actions, G_N_ELEMENTS(playlist_actions), pltab);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "playlist", G_ACTION_GROUP(actions));
		g_object_unref(actions);
	} else {
		gtk_list_store_clear(pltab->store);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "playlist", NULL);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "playlist-selected", NULL);
	}
}

void pl_tab_destroy(struct sonatina_tab *tab)
{
	struct pl_tab *pltab = (struct pl_tab *) tab;

	gtk_widget_destroy(tab->widget);

	g_strfreev(pltab->columns);
	gtk_list_store_clear(pltab->store);
	g_object_unref(pltab->store);
	g_object_unref(pltab->ui);
}

void pl_set_active(struct pl_tab *pl, int pos_req)
{
	GtkTreeIter iter;
	gint id, pos;

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl->store), &iter)) {
		/* empty playlist */
		return;
	}

	do {
		gtk_tree_model_get(GTK_TREE_MODEL(pl->store), &iter, PL_ID, &id, PL_POS, &pos, -1);
		if (pos == pos_req) {
			gtk_list_store_set(pl->store, &iter, PL_WEIGHT, PANGO_WEIGHT_BOLD, -1);
		} else {
			gtk_list_store_set(pl->store, &iter, PL_WEIGHT, PANGO_WEIGHT_NORMAL, -1);
		}
	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl->store), &iter));
}

void pl_update(struct pl_tab *pl, const struct mpd_song *song)
{
	int id, pos;
	GtkTreePath *path;
	GtkTreeIter iter;
	size_t i;
	gchar *str;

	if (!song) {
		gtk_list_store_clear(pl->store);
		return;
	}

	id = mpd_song_get_id(song);
	pos = mpd_song_get_pos(song);
	path = gtk_tree_path_new_from_indices(pos, -1);
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(pl->store), &iter, path)) {
		gtk_list_store_insert(pl->store, &iter, pos);
	}
	gtk_tree_path_free(path);

	gtk_list_store_set(pl->store, &iter, PL_ID, id, PL_POS, pos, PL_WEIGHT, PANGO_WEIGHT_NORMAL, -1);

	for (i = 0; i < pl->n_columns; i++) {
		str = song_attr_format(pl->columns[i], song);
		gtk_list_store_set(pl->store, &iter, PL_COUNT + i, str, -1);
		g_free(str);
	}
}

void playlist_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	int pos;

	store = gtk_tree_view_get_model(tw);

	if (!gtk_tree_model_get_iter(store, &iter, path)) {
		MSG_WARNING("playlist item vanished");
		return;
	}

	gtk_tree_model_get(store, &iter, PL_POS, &pos, -1);

	sonatina_play(pos);
}

void playlist_row_changed_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, struct pl_tab *tab)
{
	gint *indices;
	gint id, pos;
	char from[INT_BUF_SIZE];
	char to[INT_BUF_SIZE];

	MSG_DEBUG("playlist row changed");

	gtk_tree_model_get(model, iter, PL_ID, &id, PL_POS, &pos, -1);
	indices = gtk_tree_path_get_indices(path);
	if (!path) {
		return;
	}

	if (pos == indices[0]) {
		/* position not changed */
		return;
	}

	snprintf(from, sizeof(from), "%d", id);
	snprintf(to, sizeof(to), "%d", indices[0]);
	MSG_DEBUG("moveid %s %s (pos %d)", from, to, pos);
	mpd_send(tab->mpdsource, MPD_CMD_MOVEID, from, to, NULL);
}

void pl_process_song(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	struct pl_tab *tab = (struct pl_tab *) data;
	int pos;

	if (answer->song) {
		pos = mpd_song_get_pos(answer->song);
	} else {
		pos = -1;
	}
	pl_set_active(tab, pos);
}

void pl_process_pl(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data)
{
	GList *cur;
	struct mpd_song *song;
	struct pl_tab *tab = (struct pl_tab *) data;

	pl_update(tab, NULL);
	for (cur = answer->plinfo.list; cur; cur = cur->next) {
		song = (struct mpd_song *) cur->data;
		pl_update(tab, song);
	}
}

void pl_selection_changed(GtkTreeSelection *selection, gpointer data)
{
	struct pl_tab *tab = (struct pl_tab *) data;
	GtkTreeModel *model;
	GList *rows;
	GtkTreeView *tw;
	GSimpleActionGroup *actions;

	tw = gtk_tree_selection_get_tree_view(selection);
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (rows) {
		actions = g_simple_action_group_new();
		g_action_map_add_action_entries(G_ACTION_MAP(actions), playlist_selected_actions, G_N_ELEMENTS(playlist_selected_actions), tab);
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "playlist-selected", G_ACTION_GROUP(actions));
		g_object_unref(actions);
	} else {
		gtk_widget_insert_action_group(GTK_WIDGET(tw), "playlist-selected", NULL);
	}

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

void playlist_remove_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	struct pl_tab *tab = (struct pl_tab *) data;
	GObject *tw;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *rows;
	GList *row;
	gint id;
	char buf[INT_BUF_SIZE];

	MSG_INFO("Remove action activated");

	tw = gtk_builder_get_object(tab->ui, "tw");
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tw));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (row = rows; row; row = row->next) {
		if (!gtk_tree_model_get_iter(model, &iter, row->data)) {
			continue;
		}
		gtk_tree_model_get(model, &iter, PL_ID, &id, -1);
		snprintf(buf, sizeof(buf), "%d", id);
		mpd_send(tab->mpdsource, MPD_CMD_DELETEID, buf, NULL);
	}

	g_list_free_full(rows, (GDestroyNotify) gtk_tree_path_free);
}

void playlist_clear_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	struct pl_tab *tab = (struct pl_tab *) data;

	MSG_INFO("Clear action activated");

	mpd_send(tab->mpdsource, MPD_CMD_CLEAR, NULL);
}

void playlist_shuffle_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	struct pl_tab *tab = (struct pl_tab *) data;

	MSG_INFO("Shuffle action activated");

	mpd_send(tab->mpdsource, MPD_CMD_SHUFFLE, NULL);
}

