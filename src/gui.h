#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

void add_connected_entries();
void remove_connected_entries();

void app_startup_cb(GtkApplication *app, gpointer user_data);
void app_shutdown_cb(GtkApplication *app, gpointer user_data);
void app_activate_cb(GtkApplication *app, gpointer user_data);
gint app_options_cb(GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data);
gint app_local_options_cb(GApplication *app, GVariantDict *dict, gpointer user_data);

void prev_cb(GtkWidget *w, gpointer data);
void next_cb(GtkWidget *w, gpointer data);
void play_cb(GtkWidget *w, gpointer data);
void pause_cb(GtkWidget *w, gpointer data);
void stop_cb(GtkWidget *w, gpointer data);

gboolean timeline_clicked_cb(GtkWidget *w, GdkEvent *event, gpointer data);

/**
  @brief GTK callback for 'popup' signal that dispays sonatina menu.
  @param w Widget that emitted the signal.
  @param specific Context-specific part of the menu.
  */
void sonatina_popup_cb(GtkWidget *w, GMenuModel *specific);

/**
  @brief GTK callback for 'button-press-event' signal that displays sonatina
  menu on right button click.
  @param w Widget that emitted the signal.
  @param event Button press event.
  @param specific Context-specific part of the menu.
  */
gboolean sonatina_click_cb(GtkWidget *w, GdkEventButton *event, GMenuModel *specific);

/**
  @brief Create sonatina menu consisting of a global section and an optional
  context-specific section.
  @param specific Context-specific section of the menu.
  @returns GtkMenu widget.
  */
GtkWidget *sonatina_menu(GMenuModel *specific);

/**
  @brief Convenience function to connect popup related signals of a widget.
  @param w Widget.
  @param menu Context specific part of the menu that will be displayed.
  */
void connect_popup(GtkWidget *w, GMenuModel *menu);
void connect_popup_tw(GtkWidget *w, GMenuModel *menu);

void connect_signals();

/* Actions */
void connect_action(GSimpleAction *action, GVariant *param, gpointer data);
void settings_action(GSimpleAction *action, GVariant *param, gpointer data);
void quit_action(GSimpleAction *action, GVariant *param, gpointer data);
void db_update_action(GSimpleAction *action, GVariant *param, gpointer data);
void playlist_save_action(GSimpleAction *action, GVariant *param, gpointer data);

/**
  @brief Set text of labels in Sonatina's header.
  @param title First line.
  @param subtitle Second line.
  */
void sonatina_set_labels(const char *title, const char *subtitle);

/**
  @brief Callback for check button used in settings dialog. It sets a boolean
  type settings entry.
  @param button Widget that emitted the signal.
  @param data Pointer to struct settings_entry of the corresponding setting.
  */
void settings_toggle_cb(GtkToggleButton *button, gpointer data);

/**
  @brief Callback for GtkEntry used in settings dialog. It sets a string
  type settings entry.
  @param w Widget that emitted the signal.
  @param data Pointer to struct settings_entry of the corresponding setting.
  */
void settings_entry_cb(GtkEntry *w, gpointer data);

gboolean append_settings_toggle(GtkGrid *grid, const char *section, const char *name);
gboolean append_settings_text(GtkGrid *grid, const char *section, const char *name);

void prepare_settings_dialog();
void chooser_update();
void chooser_changed_cb(GtkComboBox *combo, gpointer data);
void profile_entry_cb(GtkEntry *w, gpointer data);
void add_profile_cb(GtkButton *button, gpointer data);
void remove_profile_cb(GtkButton *button, gpointer data);

/**
  @brief Show a dialog with one entry field with label. After text is entered
  and confirmed, it calls a defined action on the application with the entered
  text as a parameter.
  @param action Name of an action that takes one string parameter.
  @param title Title of the dialog window.
  @param label_text Label that will be displayed before the entry with ': '
  appended.
  @param entry_text Default text for the entry.
  */
void entry_dialog(const char *action, const char *title, const char *label_text, const char *entry_text);

#endif
