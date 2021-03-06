#include <glib.h>
#include <gtk/gtk.h>

#include "gui.h"
#include "client.h"
#include "core.h"
#include "util.h"
#include "settings.h"
#include "profile.h"
#include "gettext.h"

#define UIFILE DATADIR "/" PACKAGE "/" PACKAGE ".ui"
#define SETTINGS_UIFILE DATADIR "/" PACKAGE "/" "settings.ui"

#define WIDGET_MARGIN 4

static GActionEntry app_entries[] = {
	{ "connect", connect_action, "s", "\"\"", connect_action },
	{ "settings", settings_action, NULL, NULL, NULL },
	{ "quit", quit_action, NULL, NULL, NULL },
};

static GActionEntry app_connected_entries[] = {
	{ "dbupdate", db_update_action, NULL, NULL, NULL },
	{ "save", playlist_save_action, "s", NULL, NULL },
	{ "repeat", repeat_action, NULL, "false", toggle_action_changed },
	{ "random", random_action, NULL, "false", toggle_action_changed },
	{ "single", single_action, NULL, "false", toggle_action_changed },
	{ "consume", consume_action, NULL, "false", toggle_action_changed },
};

GtkBuilder *settings_ui = NULL;

void add_connected_entries()
{
	GApplication *app;

	app = g_application_get_default();
	g_assert(app != NULL);

	g_action_map_add_action_entries(G_ACTION_MAP(app), app_connected_entries, G_N_ELEMENTS(app_connected_entries), app);
}

void remove_connected_entries()
{
	GApplication *app;
	size_t i;

	app = g_application_get_default();
	g_assert(app != NULL);

	for (i = 0; i < G_N_ELEMENTS(app_connected_entries); i++) {
		g_action_map_remove_action(G_ACTION_MAP(app), app_connected_entries[i].name);
	}
}

void app_startup_cb(GtkApplication *app, gpointer user_data)
{
	GObject *win;
	gchar *profile;

	MSG_DEBUG("app_startup_cb()");

	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), DATADIR "/icons");

	sonatina.gui = gtk_builder_new_from_file(UIFILE);

	if (!settings_ui) {
		settings_ui = gtk_builder_new_from_file(SETTINGS_UIFILE);
	}

	sonatina_init();

	win = gtk_builder_get_object(sonatina.gui, "window");
	g_signal_connect(win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	gtk_application_add_window(app, GTK_WINDOW(win));

	connect_signals();
	prepare_settings_dialog();

	/* keep application running after main window is closed */
	g_application_hold(G_APPLICATION(app));

	g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);
	profile = sonatina_settings_get_string("main", "active_profile");
	g_action_group_activate_action(G_ACTION_GROUP(app), "connect", g_variant_new("s", profile));
	g_free(profile);
}

void app_shutdown_cb(GtkApplication *app, gpointer user_data)
{
	GObject *w;

	MSG_DEBUG("app_shutdown_cb()");

	sonatina_disconnect();
	sonatina_destroy();

	w = gtk_builder_get_object(sonatina.gui, "window");
	gtk_widget_destroy(GTK_WIDGET(w));

	g_object_unref(sonatina.gui);
	g_timer_destroy(sonatina.counter);

	w = gtk_builder_get_object(settings_ui, "settings_dialog");
	gtk_widget_destroy(GTK_WIDGET(w));
	g_object_unref(settings_ui);
}

void app_activate_cb(GtkApplication *app, gpointer user_data)
{
	GObject *win;

	MSG_DEBUG("app_activate_cb()");

	win = gtk_builder_get_object(sonatina.gui, "window");
	gtk_widget_show(GTK_WIDGET(win));
}

gint app_options_cb(GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data)
{
	GVariantDict *dict;
	GVariant *value;
	gchar *str;

	dict = g_application_command_line_get_options_dict(cmdline);

	value = g_variant_dict_lookup_value(dict, "kill", NULL);
	if (value) {
		MSG_DEBUG("--kill");
		g_application_quit(app);
	}

	if (g_variant_dict_lookup(dict, "profile", "s", &str)) {
		MSG_DEBUG("--profile %s", str);
		g_action_group_activate_action(G_ACTION_GROUP(app), "connect", g_variant_new("s", str));
		g_free(str);
	}

	g_application_activate(app);

	return 0;
}

gint app_local_options_cb(GApplication *app, GVariantDict *dict, gpointer user_data)
{
	GVariant *value;

	value = g_variant_dict_lookup_value(dict, "verbose", NULL);
	if (value) {
		MSG_DEBUG("--verbose");
		log_level = LEVEL_INFO;
	}

	return -1;
}

void prev_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("prev_cb");
	mpd_send(sonatina.mpdsource, MPD_CMD_PREV, NULL);
}

void next_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("next_cb");
	mpd_send(sonatina.mpdsource, MPD_CMD_NEXT, NULL);
}

void play_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("play_cb");
	mpd_send(sonatina.mpdsource, MPD_CMD_PLAY, NULL);
}

void pause_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("pause_cb");
	mpd_send(sonatina.mpdsource, MPD_CMD_PAUSE, NULL);
}

void stop_cb(GtkWidget *w, gpointer data)
{
	MSG_DEBUG("stop_cb");
	mpd_send(sonatina.mpdsource, MPD_CMD_STOP, NULL);
}

void volume_cb(GtkWidget *w, gpointer data)
{
	gdouble scale;

	MSG_DEBUG("volume_cb");
	scale = gtk_scale_button_get_value(GTK_SCALE_BUTTON(w));
	sonatina_setvol(scale);
}

gboolean timeline_clicked_cb(GtkWidget *w, GdkEvent *event, gpointer data)
{
	gdouble x;
	int width, time;

	MSG_DEBUG("timeline_clicked_cb()");

	x = ((GdkEventButton *) event)->x;
	width = gtk_widget_get_allocated_width(w);

	if (width == 0) time = 0;
	else time = (x / width) * sonatina.total;
	sonatina_seek(time);

	return FALSE;
}

GtkWidget *sonatina_menu(GMenuModel *specific)
{
	GtkWidget *menuw;
	GMenu *menu;
	GObject *sonatina_menu;
	GObject *connmenu;
	GList *profile;
	const gchar *name;
	gchar *action;

	connmenu = gtk_builder_get_object(sonatina.gui, "connmenu");
	g_menu_remove_all(G_MENU(connmenu));
	for (profile = profiles; profile; profile = profile->next) {
		name = ((const struct sonatina_profile *) profile->data)->name;
		action = g_strdup_printf("%s::%s", "app.connect", name);
		g_menu_append(G_MENU(connmenu), name, action);
		g_free(action);
	}

	g_menu_append(G_MENU(connmenu), _("Disconnect"), "app.connect::");

	sonatina_menu = gtk_builder_get_object(sonatina.gui, "menu");
	menu = g_menu_new();
	g_menu_prepend_section(menu, NULL, G_MENU_MODEL(sonatina_menu));

	if (specific) {
		g_menu_prepend_section(menu, NULL, specific);
	}

	menuw = gtk_menu_new_from_model(G_MENU_MODEL(menu));
	g_object_unref(menu);

	return menuw;
}

void sonatina_popup_menu(GtkWidget *w, GdkEventButton *event, GMenuModel *specific)
{
	GtkWidget *menu;
	int button, time;

	MSG_INFO("popup menu");

	menu = sonatina_menu(specific);
	if (event) {
		button = event->button;
		time = event->time;
	} else {
		button = 0;
		time = gtk_get_current_event_time();
	}
	gtk_menu_attach_to_widget (GTK_MENU (menu), w, NULL);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, time);
}

void sonatina_popup_cb(GtkWidget *w, GMenuModel *specific)
{
	sonatina_popup_menu(w, NULL, specific);
}

gboolean sonatina_click_cb(GtkWidget *w, GdkEventButton *event, GMenuModel *specific)
{
	GtkTreeSelection *selection;
	GtkTreePath *path;

	if (gdk_event_triggers_context_menu ((GdkEvent *) event) && event->type == GDK_BUTTON_PRESS) {
		if (!g_strcmp0(G_OBJECT_TYPE_NAME(w), "GtkTreeView")) {
			/* Special handling of GtkTreeView */
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
			if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
				if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w),
						event->x, event->y,
						&path, NULL, NULL, NULL) ) {
					gtk_tree_selection_unselect_all(selection);
					gtk_tree_selection_select_path(selection, path);
				}
			}
		}
		sonatina_popup_menu(w, event, specific);
		return TRUE;
	}
	
	return FALSE;
}

void connect_popup(GtkWidget *w, GMenuModel *menu)
{
	g_signal_connect(G_OBJECT(w), "popup-menu", G_CALLBACK(sonatina_popup_cb), menu);
	g_signal_connect(G_OBJECT(w), "button-press-event", G_CALLBACK(sonatina_click_cb), menu);
}

void connect_signals()
{
	GObject *w;

	/* context menu */
	w = gtk_builder_get_object(sonatina.gui, "window");
	connect_popup(GTK_WIDGET(w), NULL);

	/* playback buttons */
	w = gtk_builder_get_object(sonatina.gui, "prev_button");
	g_signal_connect(w, "clicked", G_CALLBACK(prev_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "play_button");
	g_signal_connect(w, "clicked", G_CALLBACK(play_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "pause_button");
	g_signal_connect(w, "clicked", G_CALLBACK(pause_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "stop_button");
	g_signal_connect(w, "clicked", G_CALLBACK(stop_cb), NULL);
	w = gtk_builder_get_object(sonatina.gui, "next_button");
	g_signal_connect(w, "clicked", G_CALLBACK(next_cb), NULL);

	/* volume button */
	w = gtk_builder_get_object(sonatina.gui, "volbutton");
	g_signal_connect(w, "value-changed", G_CALLBACK(volume_cb), NULL);

	/* timeline */
	w = gtk_builder_get_object(sonatina.gui, "timeline_eb");
	gtk_widget_add_events(GTK_WIDGET(w), GDK_BUTTON_PRESS_MASK);
	g_signal_connect(w, "button-press-event", G_CALLBACK(timeline_clicked_cb), NULL);

	/* settings */
	w = gtk_builder_get_object(settings_ui, "profile_name_entry");
	g_signal_connect(w, "changed", G_CALLBACK(profile_entry_cb), NULL);
	w = gtk_builder_get_object(settings_ui, "host_entry");
	g_signal_connect(w, "changed", G_CALLBACK(profile_entry_cb), NULL);
	w = gtk_builder_get_object(settings_ui, "port_entry");
	g_signal_connect(w, "changed", G_CALLBACK(profile_entry_cb), NULL);
	w = gtk_builder_get_object(settings_ui, "password_entry");
	g_signal_connect(w, "changed", G_CALLBACK(profile_entry_cb), NULL);

	w = gtk_builder_get_object(settings_ui, "profile_add_button");
	g_signal_connect(w, "clicked", G_CALLBACK(add_profile_cb), NULL);
	w = gtk_builder_get_object(settings_ui, "profile_remove_button");
	g_signal_connect(w, "clicked", G_CALLBACK(remove_profile_cb), NULL);
}

void connect_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	const gchar *profile;
	gsize len;

	profile = g_variant_get_string(param, &len);
	if (len) {
		if (!sonatina_change_profile_by_name(profile)) {
			MSG_WARNING("Invalid profile '%s'", profile);
			return;
		}
	} else {
		sonatina_disconnect();
	}
	g_simple_action_set_state(action, param);
}

void settings_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	GObject *win;

	MSG_INFO("Settings activated");

	win = gtk_builder_get_object(settings_ui, "settings_dialog");
	gtk_widget_show_all(GTK_WIDGET(win));
}

void quit_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	g_application_quit(G_APPLICATION(data));
}

void db_update_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	MSG_INFO("Update action activated");

	if (sonatina.mpdsource) {
		mpd_send(sonatina.mpdsource, MPD_CMD_UPDATE, NULL);
	}
}

void playlist_save_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	const gchar *name;
	gsize len;

	MSG_INFO("Save action activated");

	name = g_variant_get_string(param, &len);
	if (len) {
		mpd_send(sonatina.mpdsource, MPD_CMD_SAVE, name, NULL);
	} else {
		entry_dialog("save", _("Save playlist"), _("Playlist name"), _("New playlist"));
	}
}

void repeat_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	GVariant *state;
	gboolean val;

	MSG_INFO("Repeat action activated");

	state = g_action_get_state(G_ACTION(action));
	val = g_variant_get_boolean(state);
	mpd_send(sonatina.mpdsource, MPD_CMD_REPEAT, mpd_bool_str(!val), NULL);
}

void toggle_action_changed(GSimpleAction *action, GVariant *state, gpointer data)
{
	g_simple_action_set_state(action, state);
}

void random_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	GVariant *state;
	gboolean val;

	MSG_INFO("Random action activated");

	state = g_action_get_state(G_ACTION(action));
	val = g_variant_get_boolean(state);
	mpd_send(sonatina.mpdsource, MPD_CMD_RANDOM, mpd_bool_str(!val), NULL);
}

void single_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	GVariant *state;
	gboolean val;

	MSG_INFO("Random action activated");

	state = g_action_get_state(G_ACTION(action));
	val = g_variant_get_boolean(state);
	mpd_send(sonatina.mpdsource, MPD_CMD_SINGLE, mpd_bool_str(!val), NULL);
}

void consume_action(GSimpleAction *action, GVariant *param, gpointer data)
{
	GVariant *state;
	gboolean val;

	MSG_INFO("Random action activated");

	state = g_action_get_state(G_ACTION(action));
	val = g_variant_get_boolean(state);
	mpd_send(sonatina.mpdsource, MPD_CMD_CONSUME, mpd_bool_str(!val), NULL);
}

void sonatina_set_labels(const char *title, const char *subtitle)
{
	GObject *label;

	if (title) {
		label = gtk_builder_get_object(sonatina.gui, "title");
		gtk_label_set_text(GTK_LABEL(label), title);
	}

	if (subtitle) {
		label = gtk_builder_get_object(sonatina.gui, "subtitle");
		gtk_label_set_text(GTK_LABEL(label), subtitle);
	}
}

void settings_toggle_cb(GtkToggleButton *button, gpointer data)
{
	const struct settings_entry *entry = (const struct settings_entry *) data;
	union settings_value val;

	g_assert(entry != NULL);

	val.boolean = gtk_toggle_button_get_active(button);
	sonatina_settings_set(entry->section, entry->name, val);
}

void settings_entry_cb(GtkEntry *w, gpointer data)
{
	const struct settings_entry *entry = (const struct settings_entry *) data;
	union settings_value val;

	g_assert(entry != NULL);

	val.string = g_strdup(gtk_entry_get_text(w));
	sonatina_settings_set(entry->section, entry->name, val);
	g_free(val.string);
}

gboolean append_settings_toggle(GtkGrid *grid, const char *section, const char *name)
{
	GtkWidget *cb;
	const struct settings_entry *e;
	union settings_value val;

	e = settings_lookup(section, name, SETTINGS_BOOL);

	if (!e) {
		MSG_ERROR("Couldn't create toggle widget for settings entry %s/%s: undefine settings entry", section, name);
		return FALSE;
	}

	sonatina_settings_get(section, name, &val);

	cb = gtk_check_button_new_with_label(e->label);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), val.boolean);
	g_object_set(G_OBJECT(cb), "margin", WIDGET_MARGIN, NULL);

	g_signal_connect(G_OBJECT(cb), "toggle", G_CALLBACK(settings_toggle_cb), (gpointer) e);

	gtk_grid_attach_next_to(grid, cb, NULL, GTK_POS_BOTTOM, 1, 1);

	return TRUE;
}

gboolean append_settings_text(GtkGrid *grid, const char *section, const char *name)
{
	GtkWidget *label;
	GtkWidget *entry;
	const struct settings_entry *e;
	union settings_value val;

	e = settings_lookup(section, name, SETTINGS_STRING);

	if (!e) {
		MSG_ERROR("Couldn't create entry widget for settings entry %s/%s: undefine settings entry", section, name);
		return FALSE;
	}

	sonatina_settings_get(section, name, &val);

	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), val.string);
	gtk_widget_set_hexpand(entry, TRUE);
	label = gtk_label_new(e->label);
	g_object_set(G_OBJECT(label), "margin", WIDGET_MARGIN, NULL);
	gtk_widget_set_halign(label, GTK_ALIGN_START);

	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(settings_entry_cb), (gpointer) e);

	gtk_grid_attach_next_to(grid, label, NULL, GTK_POS_BOTTOM, 1, 1);
	gtk_grid_attach_next_to(grid, entry, label, GTK_POS_RIGHT, 1, 1);

	return TRUE;
}

void prepare_settings_dialog()
{
	GObject *dialog;
	GObject *parent;
	GObject *grid;
	GObject *chooser;
	GtkCellRenderer *renderer;
	GObject *model;

	dialog = gtk_builder_get_object(settings_ui, "settings_dialog");
	g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_hide), dialog);

	/* set dialog's parent */
	parent = gtk_builder_get_object(sonatina.gui, "window");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));

	/* Format frame */
	grid = gtk_builder_get_object(settings_ui, "format_grid");
	append_settings_text(GTK_GRID(grid), "playlist", "format");
	append_settings_text(GTK_GRID(grid), "library", "format");
	append_settings_text(GTK_GRID(grid), "main", "title");
	append_settings_text(GTK_GRID(grid), "main", "subtitle");

	model = gtk_builder_get_object(settings_ui, "profile_chooser_model");
	chooser = gtk_builder_get_object(settings_ui, "profile_chooser");
	renderer = gtk_cell_renderer_text_new();
	gtk_combo_box_set_model(GTK_COMBO_BOX(chooser), GTK_TREE_MODEL(model));
	gtk_combo_box_set_id_column(GTK_COMBO_BOX(chooser), 0);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(chooser), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(chooser), renderer, "text", 0, NULL);
	g_signal_connect(chooser, "changed", G_CALLBACK(chooser_changed_cb), NULL);
	chooser_update();
}

void chooser_changed_cb(GtkComboBox *combo, gpointer data)
{
	GObject *entry;
	const struct sonatina_profile *profile;

	profile = sonatina_get_profile(gtk_combo_box_get_active_id(combo));

	if (!profile) {
		return;
	}

	entry = gtk_builder_get_object(settings_ui, "profile_name_entry");
	gtk_entry_set_text(GTK_ENTRY(entry), profile->name);

	entry = gtk_builder_get_object(settings_ui, "host_entry");
	gtk_entry_set_text(GTK_ENTRY(entry), profile->host);

	entry = gtk_builder_get_object(settings_ui, "port_entry");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(entry), profile->port);

	if (profile->password) {
		entry = gtk_builder_get_object(settings_ui, "password_entry");
		gtk_entry_set_text(GTK_ENTRY(entry), profile->password);
	}
}

void chooser_update()
{
	GObject *chooser;
	GtkTreeModel *model;
	gint selected;
	gint n_items = 0;
	GList *profile;
	const char *name;

	chooser = gtk_builder_get_object(settings_ui, "profile_chooser");
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(chooser));
	selected = gtk_combo_box_get_active(GTK_COMBO_BOX(chooser));

	gtk_list_store_clear(GTK_LIST_STORE(model));

	for (profile = profiles; profile; profile = profile->next) {
		name = ((struct sonatina_profile *) profile->data)->name;
		gtk_list_store_insert_with_values(GTK_LIST_STORE(model), NULL, -1, 0, name, -1);
		n_items++;
	}

	if (selected >= n_items) {
		selected = n_items - 1;
	}

	if (selected < 0) {
		selected = 0;
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(chooser), selected);
}

void profile_entry_cb(GtkEntry *w, gpointer data)
{
	GObject *chooser;
	GtkTreeIter iter;
	const gchar *id;
	const gchar *profile;
	const gchar *name = NULL;
	const gchar *host = NULL;
	const gchar *password = NULL;
	gint port = -1;

	chooser = gtk_builder_get_object(settings_ui, "profile_chooser");
	profile = gtk_combo_box_get_active_id(GTK_COMBO_BOX(chooser));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(chooser), &iter)) {
		return;
	}

	id = gtk_buildable_get_name(GTK_BUILDABLE(w));
	MSG_DEBUG("entry '%s' activated", id);

	if (!g_strcmp0(id, "profile_name_entry")) {
		name = gtk_entry_get_text(GTK_ENTRY(w));
	} else if (!g_strcmp0(id, "host_entry")) {
		host = gtk_entry_get_text(GTK_ENTRY(w));
	} else if (!g_strcmp0(id, "port_entry")) {
		port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
	} else if (!g_strcmp0(id, "password_entry")) {
		password = gtk_entry_get_text(GTK_ENTRY(w));
	}

	sonatina_modify_profile(profile, name, host, port, password);
}

#define DEFAULT_NEW_PROFILE _("New profile")
#define DEFAULT_NEW_PROFILE_N _("New profile %d")
#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 6600
#define DEFAULT_PASSWORD ""

gchar *create_profile_name()
{
	gchar *name;
	int i;

	if (!sonatina_get_profile(DEFAULT_NEW_PROFILE)) {
		return g_strdup(DEFAULT_NEW_PROFILE);
	}

	for (i = 1; i <= INT_MAX; i++) {
		name = g_strdup_printf(DEFAULT_NEW_PROFILE_N, i);
		if (!sonatina_get_profile(name)) {
			return name;
		}
	}

	return NULL;
}

void add_profile_cb(GtkButton *button, gpointer data)
{
	GObject *chooser;
	gchar *name;

	chooser = gtk_builder_get_object(settings_ui, "profile_chooser");
	name = create_profile_name();

	if (!name) {
		MSG_ERROR("Coudn't create new profile");
		return;
	}

	sonatina_add_profile(name, DEFAULT_HOST, DEFAULT_PORT, DEFAULT_PASSWORD);
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(chooser), name);
	g_free(name);
}

void remove_profile_cb(GtkButton *button, gpointer data)
{
	GObject *chooser;
	const gchar *name;

	chooser = gtk_builder_get_object(settings_ui, "profile_chooser");
	name = gtk_combo_box_get_active_id(GTK_COMBO_BOX(chooser));

	sonatina_remove_profile(name);
}

void entry_dialog_response(GtkDialog *dialog, gint response, GtkEntry *entry)
{
	const gchar *name;
	gchar *action;
	GApplication *app;

	MSG_DEBUG("entry_dialog_response()");

	app = g_application_get_default();
	g_assert(app != NULL);

	action = g_object_get_data(G_OBJECT(dialog), "action");

	if (response == GTK_RESPONSE_OK) {
		MSG_DEBUG("response OK");
		name = gtk_entry_get_text(entry);
		g_action_group_activate_action(G_ACTION_GROUP(app), action, g_variant_new("s", name));
	}

	g_free(action);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void entry_dialog(const char *action, const char *title, const char *label_title, const char *entry_text)
{
	GObject *win;
	GtkWidget *dialog;
	GtkWidget *okbutton;
	GtkWidget *box;
	GtkWidget *label;
	gchar *label_text;
	GtkWidget *entry;

	win = gtk_builder_get_object(sonatina.gui, "window");

	dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(win),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			_("Cancel"), GTK_RESPONSE_REJECT,
			_("OK"), GTK_RESPONSE_OK,
			NULL);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	label_text = g_strdup_printf(_("%s: "), label_title);
	label = gtk_label_new(label_text);
	g_free(label_text);
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), entry_text);
	g_object_set_data(G_OBJECT(dialog), "action", g_strdup(action));
	g_signal_connect(dialog, "response", G_CALLBACK(entry_dialog_response), entry);

	g_object_set(G_OBJECT(label), "margin", WIDGET_MARGIN, NULL);
	g_object_set(G_OBJECT(entry), "margin", WIDGET_MARGIN, NULL);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(entry), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);

	gtk_window_set_focus(GTK_WINDOW(dialog), NULL);
	okbutton = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_widget_set_can_default(okbutton, TRUE);
	gtk_window_set_default(GTK_WINDOW(dialog), okbutton);
	gtk_widget_show_all(dialog);
}
