#ifndef CORE_H
#define CORE_H

#include <glib.h>
#include <gtk/gtk.h>

#include "mpd/client.h"

#include "client.h"
#include "profile.h"

/**
  @brief Structure holding data of a running sonatina instance.
  */
struct sonatina_instance {
	GSource *mpdsource; /** NULL when not connected */

	GtkBuilder *gui;
	GList *tabs;

	int cur;
	int elapsed;
	int total;
	GTimer *counter;
};

/**
  Structure representing one tab in sonatina's notebook widget.
  */
struct sonatina_tab {
	gchar *name; /** Internal name of the tab */
	gchar *label; /** Translatable name of the tab that will be displayed as the tab's label */
	GtkWidget *widget; /** Top-level widget of the tab */
	gboolean (*init)(struct sonatina_tab *); /** Function to initialize tab;
						   called when a tab is added to
						   the notebook. Returns TRUE on
						  success, FALSE otherwise. When
						  succeeded, this function
						  should set widget to a valid
						  GtkWidget. */
	void (*set_mpdsource)(struct sonatina_tab *, GSource *); /** Function
								   called when
								   sonatina's
								   MPD
								   connection is
								   changed.
								   Setting MPD
								   source to
								   NULL means
								   disconnect. */
	void (*destroy)(struct sonatina_tab *); /** Cleanup function to free memory allocated by init function */
};

typedef gboolean (*TabInitFunc)(struct sonatina_tab *);
typedef void (*TabSetSourceFunc)(struct sonatina_tab *, GSource *);
typedef void (*TabDestroyFunc)(struct sonatina_tab *);

/**
  @brief Create a new tab.
  @param name A name that will be used internally for the tab.
  @param label A human-readable name for the tab, that will be used as a label.
  @param size Size of the tab structure. Must be at least sizeof(struct sonatina_tab).
  @param init Function to initialize the tab.
  @param destroy Function free memory allocated by init function.
  @returns A newly allocated tab that should be freed with g_free().
  */
struct sonatina_tab *sonatina_tab_new(const char *name, const char *label, size_t size, TabInitFunc init, TabSetSourceFunc set_source, TabDestroyFunc destroy);

void sonatina_tab_destroy(struct sonatina_tab *tab);

/**
  @brief Append a newly created tab to sonatina's list of tabs and call its initialization function.
  @param tab Tab to append.
  @returns TRUE if tab was successfully initialized and appended to list.
  */
gboolean sonatina_append_tab(struct sonatina_tab *tab);
gboolean sonatina_remove_tab(const char *name);

extern struct sonatina_instance sonatina;

void sonatina_init();
void sonatina_destroy();

/**
  @brief Connect sonatina instance to a MPD server.
  @param host Host name.
  @param port TCP port.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_connect(const char *host, int port);

/**
  @brief Disconnect sonatina instance from MPD server.
  */
void sonatina_disconnect();

/**
  @brief Change connection profile on sonatina instance.
  @param profile Profile structure that should be part of sonatina's profile
  list. If profile is NULL, just disconnect.
  @returns TRUE when profile was successfully changed.
  */
gboolean sonatina_change_profile(const struct sonatina_profile *profile);

/**
  @brief Change connection profile on sonatina instance.
  @param name Name of the profile. If profile is NULL, just disconnect.
  @returns TRUE when profile was successfully changed.
  */
gboolean sonatina_change_profile_by_name(const char *name);

/**
  @brief Callback for MPD command currentsong. Update current song on a sonatina instance.
  @param cmd MPD command type.
  @param args MPD command argument list.
  @param answer Answer to command.
  @param data Pointer to library tab.
  */
void sonatina_update_song(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data);

/**
  @brief Callback for MPD command status. Update status on a sonatina instance.
  @param cmd MPD command type.
  @param args MPD command argument list.
  @param answer Answer to command.
  @param data Pointer to library tab.
  */
void sonatina_update_status(enum mpd_cmd_type cmd, GList *args, union mpd_cmd_answer *answer, void *data);

/**
  @brief Function called every second to check elapsed time and update progressbar.
  @param data Unused user data.
  @returns TRUE
  */
gboolean counter_cb(gpointer data);

/**
  @brief Convenience function to send mpd command 'play' with integer argument.
  @param pos Position attribute of the song to be played.
  */
void sonatina_play(int pos);

/**
  @brief Convenience function to send mpd command 'seekcur' with integer argument.
  @param time Time where to seek to.
  */
void sonatina_seek(int time);

/**
  @brief Convenience function to send mpd command 'setvol' with integer argument.
  @param vol Volume as a value in range between 0 and 1.
  */
void sonatina_setvol(double vol);


#endif
