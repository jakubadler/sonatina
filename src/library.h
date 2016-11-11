#ifndef LIBRARY_H
#define LIBRARY_H

#include "client.h"
#include "core.h"
#include "pathbar.h"

enum listing_type {
	LIBRARY_FS,
	LIBRARY_GENRE,
	LIBRARY_ARTIST,
	LIBRARY_ALBUM,
	LIBRARY_SONG,
	LIBRARY_COUNT
};

/**
  Structure representing a node in a directory tree. It can be used to
  construct single path in such tree.
  */
struct library_path {
	enum listing_type type; /** Type of the entry. All types are "directory"
				  except for LIBRARY_SONG that is a leaf in
				  directory tree. */
	struct library_path *parent; /** Link to parent node or NULL if this is
				       root*/
	struct library_path *next; /** Link to child node if it is opened or
				     NULL */
	
	char *name; /** Name of this node that will be presented to the user */
	int selected; /** Index of selected item in this directory or negative
			value if nothing is selected. */
};

/**
  Structure derivated from struct sonatina_tab.
  */
struct library_tab {
	struct sonatina_tab tab;
	GtkBuilder *ui; /** GTK builder containing UI definitons of the library
			 tab */
	GSource *mpdsource; /** Connected MPD source or NULL */
	GtkListStore *store; /** Model for the library TreeView */
	SonatinaPathBar *pathbar; /** Path bar widget */
	struct library_path *root; /** Root of the browsed tree */
	struct library_path *path; /** Currently opened node of the browsed tree
				     */
};

/**
  Columns of the list store
  */
enum lib_columns {
	LIB_COL_NAME,
	LIB_COL_ICON,
	LIB_COL_URI,
	LIB_COL_COUNT
};

/**
  @brief Initialize library tab.
  @param tab Tab to initialize.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean library_tab_init(struct sonatina_tab *tab);

/**
  @brief Set MPD source of a library tab.
  @param tab Library tab.
  @param source MPD source.
  */
void library_tab_set_source(struct sonatina_tab *tab, GSource *source);

/**
  @brief Free memory allocataed by library tab.
  @param tab Library tab.
  */
void library_tab_destroy(struct sonatina_tab *tab);

/**
  @brief Set columns of library tree view.
  @param tw Tree view.
  */
void library_tw_set_columns(GtkTreeView *tw);

/**
  @brief Open directory identified by name in the currently opened library tree
  node.
  @param tab Library tab.
  @param name Name of the item to be opened.
  */
void library_tab_open_dir(struct library_tab *tab, const char *name);

/**
  @brief Open a directory that is part of the current path.
  @param tab Library tab.
  @param pos Distance from root.
  */
void library_tab_open_pos(struct library_tab *tab, int pos);

/**
  @brief Callback for list command.
  @param args MPD command argument list.
  @param answer Answer to command.
  @param data Pointer to library tab.
  */
void library_list_cb(GList *args, union mpd_cmd_answer *answer, void *data);

/**
  @brief Callback for lsinfo command. lsinfo is used to retrieve directory
  contents or song list.
  @param args MPD command argument list.
  @param answer Answer to command.
  @param data Pointer to library tab.
  */
void library_lsinfo_cb(GList *args, union mpd_cmd_answer *answer, void *data);

/**
  @brief Callback for pathbar widget's 'changed' signal.
  @param pathbar Path bar widget.
  @param selected Index of node that was selected.
  @param data pointer to library tab.
  */
void library_pathbar_changed(SonatinaPathBar *pathbar, gint selected, gpointer data);

/**
  @brief Set listing type for library tab.
  @param tab Library tab.
  @param listing Listing type.
  */
void library_set_listing(struct library_tab *tab, enum listing_type listing);

/**
  @brief Send MPD commands to update library tab's state.
  @param tab Library tab.
  @returns TRUE when a command was successfully sent, FALSE otherwise.
  */
gboolean library_load(struct library_tab *tab);

/**
  @brief Add item to current playlist.
  @param tab Library tab.
  @param name Name of item in current library listing or URI if it's a song or
  directory.
  @returns TRUE when a command was successfully sent, FALSE otherwise.
  */
gboolean library_add(struct library_tab *tab, const char *name);

/**
  @brief Create new path root.
  @param listing Listing type.
  @returns Newly allocated root node that should be freed with @a
  library_path_free().
  */
struct library_path *library_path_root(enum listing_type listing);

/**
  @brief Open a directory in an existing path node.
  @param path Path node.
  @param name Name of directory that should be opened.
  @returns Pointer to a newly allocated path node that is successor of @a parent
  or NULL on failure.
  */
struct library_path *library_path_open(struct library_path *path, const char *name);

/**
  @brief Free a single library path node created with @a library_path_root() or
  @a library_path_open().
  @param path Path node that shall be freed.
  */
void library_path_free(struct library_path *path);

/**
  @brief Free whole tree of path nodes.
  @param path Root of a tree that should be freed.
  */
void library_path_free_all(struct library_path *path);

/**
  @brief Callback function for library tree view.
  */
void library_clicked_cb(GtkTreeView *tw, GtkTreePath *path, GtkTreeViewColumn *col, struct library_tab *tab);

/**
  @brief Append an item specified by libmpd's entity to library list.
  @param model Gtk list store used as model for tree view.
  @param entity MPD entity.
  */
void library_model_append_entity(GtkListStore *model, const struct mpd_entity *entity);

/**
  @brief Append an item specified by type and name to library list.
  @param model GTK list store used as model for tree view.
  @param type Type of the item.
  @param name Name of the item.
  */
void library_model_append(GtkListStore *model, enum listing_type type, const char *name, const char *uri);

/**
  @brief Get path string for given path. This string doesn't begin with slash to
  be compatible with MPD lsinfo command.
  @param root Path root.
  @param path Path end.
  @return Newly allocated string that should be freed with g_free().
  */
gchar *library_path_get_uri(const struct library_path *root, const struct library_path *path);

/**
  @brief Create GTK menu for listing selector.
  @param tab Library tab.
  @returns Menu widget.
  */
GtkWidget *library_selector_menu(struct library_tab *tab);

/**
  @brief Iterate over all items in selection and call @a library_tab() to
  request adding them to playlist.
  @param tab Library tab.
  @param selection Selection on library TreeView.
  @return TRUE if all commands were successfully sent.
  */
gboolean library_add_selected(struct library_tab *tab, GtkTreeSelection *selection);

void library_selection_changed(GtkTreeSelection *selection, gpointer data);

/**
  @brief Action to add selected items from library to playlist.
  @param action Action.
  @param param Action parameter.
  @param data Pointer to library tab.
  */
void library_add_action(GSimpleAction *action, GVariant *param, gpointer data);

/**
  @brief Same as 'add' action except that it clers playlist befor adding items.
  */
void library_replace_action(GSimpleAction *action, GVariant *param, gpointer data);

void library_update_action(GSimpleAction *action, GVariant *param, gpointer data);

#endif
