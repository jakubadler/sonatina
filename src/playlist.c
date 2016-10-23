#include <gtk/gtk.h>

#include "core.h"
#include "playlist.h"
#include "songattr.h"
#include "util.h"
#include "client.h"

gboolean pl_tab_init(struct sonatina_tab *tab)
{
	struct pl_tab *pltab = (struct pl_tab *) tab;
	size_t i;
	GType *types;
	GObject *tw;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	gchar *title;
	gchar *format;

	/* load ui */
	pltab->ui = load_tab_ui(tab->name);
	if (!pltab->ui) {
		MSG_ERROR("Failed to load ui file for tab %s", tab->name);
		return FALSE;
	}

	format = g_key_file_get_string(sonatina.rc, "playlist", "format", NULL);

	pltab->n_columns = 1;
	for (i = 0; format[i]; i++) {
		if (format[i] == '|')
			pltab->n_columns++;
	}

	pltab->columns = g_strsplit(format, "|", 0);
	g_free(format);
	types = g_malloc((PL_COUNT + pltab->n_columns) * sizeof(GType));

	types[PL_ID] = G_TYPE_INT;
	types[PL_POS] = G_TYPE_INT;
	types[PL_WEIGHT] = G_TYPE_INT;
	for (i = 0; pltab->columns[i]; i++) {
		types[PL_COUNT + i] = G_TYPE_STRING;
	}
	pltab->store = gtk_list_store_newv(PL_COUNT + pltab->n_columns, types);
	g_free(types);

	/* update TreeView */
	tw = gtk_builder_get_object(pltab->ui, "tw");
	if (!tw) {
		MSG_INFO("failed to load playlist tree view");
		return FALSE;
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(tw), GTK_TREE_MODEL(pltab->store));

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "weight-set", TRUE, NULL);

	/* remove all columns */
	for (i = 0; (col = gtk_tree_view_get_column(GTK_TREE_VIEW(tw), i)); i++) {
		gtk_tree_view_remove_column(GTK_TREE_VIEW(tw), col);
	}

	/* add new columns */
	for (i = 0; pltab->columns[i]; i++) {
		title = song_attr_format(pltab->columns[i], NULL);
		MSG_DEBUG("adding column with format '%s' to playlist tw", pltab->columns[i]);
		col = gtk_tree_view_column_new_with_attributes(title, renderer, "weight", PL_WEIGHT, "text", PL_COUNT + i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tw), col);
	}

	g_signal_connect(G_OBJECT(tw), "row-activated", G_CALLBACK(playlist_clicked_cb), NULL);

	/* set tab widget */
	tab->widget = GTK_WIDGET(gtk_builder_get_object(pltab->ui, "top"));

	return TRUE;
}

void pl_tab_set_source(struct sonatina_tab *tab, GSource *source)
{
	mpd_source_register(source, MPD_CMD_CURRENTSONG, pl_process_song, tab);
	mpd_source_register(source, MPD_CMD_PLINFO, pl_process_pl, tab);
}

void pl_tab_destroy(struct sonatina_tab *tab)
{
	struct pl_tab *pltab = (struct pl_tab *) tab;

	g_object_unref(G_OBJECT(pltab->ui));
	g_strfreev(pltab->columns);
	gtk_list_store_clear(pltab->store);
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

void pl_process_song(union mpd_cmd_answer *answer, void *data)
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

void pl_process_pl(union mpd_cmd_answer *answer, void *data)
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

