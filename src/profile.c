#include <glib.h>

#include "profile.h"

#include "util.h"
#include "core.h"
#include "gui.h"

GList *profiles;

gboolean sonatina_profiles_load()
{
	gchar *profilesfile;
	GKeyFile *keyfile;
	gchar **profnames;
	size_t i;
	struct sonatina_profile *profile = NULL;
	gboolean success;
	
	profilesfile = g_build_filename(g_get_user_config_dir(), PACKAGE, "profiles.ini", NULL);
	keyfile = g_key_file_new();
	success = g_key_file_load_from_file(keyfile, profilesfile,
			G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
			NULL);
	g_free(profilesfile);

	if (!success) {
		return FALSE;
	}

	profnames = g_key_file_get_groups(keyfile, NULL);
	profiles = NULL;
	for (i = 0; profnames[i]; i++) {
		if (!profile) {
			profile = g_malloc(sizeof(struct sonatina_profile));
		}
		profile->name = g_strdup(profnames[i]);
		profile->host = g_key_file_get_string(keyfile, profnames[i], "host", NULL);
		profile->port = g_key_file_get_integer(keyfile, profnames[i], "port", NULL);
		profile->password = g_key_file_get_string(keyfile, profnames[i], "password", NULL);
		if (profile->host) {
			profiles = g_list_append(profiles, profile);
			profile = NULL;
		}
	}

	chooser_update();

	g_key_file_free(keyfile);
	g_strfreev(profnames);

	return TRUE;

}

gboolean sonatina_profiles_save()
{
	gchar *profilesfile;
	GKeyFile *keyfile;
	GList *node;
	struct sonatina_profile *profile;
	gboolean success;
	
	keyfile = g_key_file_new();

	for (node = profiles; node; node = node -> next) {
		profile = (struct sonatina_profile *) node->data;
		g_key_file_set_string(keyfile, profile->name, "host", profile->host);
		g_key_file_set_integer(keyfile, profile->name, "port", profile->port);

		if (profile->password) {
			g_key_file_set_string(keyfile, profile->name, "password", profile->password);
		}
	}

	profilesfile = g_build_filename(g_get_user_config_dir(), PACKAGE, "profiles.ini", NULL);

	success = g_key_file_save_to_file(keyfile, profilesfile, NULL);

	g_free(profilesfile);
	g_key_file_free(keyfile);
	g_list_free_full(profiles, (GDestroyNotify) sonatina_profile_free);

	return success;
}

void sonatina_add_profile(const char *name, const char *host, int port, const char *password)
{
	struct sonatina_profile *profile;

	g_assert(name != NULL);
	g_assert(host != NULL);

	profile = g_malloc(sizeof(struct sonatina_profile));

	profile->name = g_strdup(name);
	profile->host = g_strdup(host);
	profile->port = port;

	if (password) {
		profile->password = g_strdup(password);
	} else {
		profile->password = NULL;
	}

	profiles = g_list_append(profiles, profile);

	chooser_update();
}

gboolean sonatina_modify_profile(const char *name, const char *newname, const char *host, int port, const char *password)
{
	GList *node;
	struct sonatina_profile *profile;

	node = sonatina_lookup_profile(name);

	if (!node) {
		MSG_INFO("profile '%s' doesn't exist", name);
		return FALSE;
	}

	profile = (struct sonatina_profile *) node->data;

	if (newname && g_strcmp0(name, newname)) {
		if (sonatina_lookup_profile(newname)) {
			MSG_WARNING("Can't rename profile '%s' to '%s': such profile already exist", profile->name, newname);
			return FALSE;
		} else {
			MSG_DEBUG("chnging profile name form '%s' to '%s'", profile->name, newname);
			g_free(profile->name);
			profile->name = g_strdup(newname);
		}
	}

	if (host) {
		MSG_DEBUG("changing host in profile '%s' to '%s'", profile->name, host);
		g_free(profile->host);
		profile->host = g_strdup(host);
	}

	if (port >= 0) {
		MSG_DEBUG("changing port in profile '%s' to '%d'", profile->name, port);
		profile->port = port;
	}

	if (password) {
		MSG_DEBUG("changing password in profile '%s' to '%s'", profile->name, password);
		if (profile->password) {
			g_free(profile->password);
		}
		profile->password = g_strdup(password);
	}

	chooser_update();

	/* TODO: reconnect if this is current profile */

	return TRUE;
}

gboolean sonatina_remove_profile(const char *name)
{
	GList *node;

	node = sonatina_lookup_profile(name);

	if (!node) {
		MSG_INFO("profile '%s' doesn't exist", name);
		return FALSE;
	}

	profiles = g_list_remove_link(profiles, node);
	g_list_free_full(node, (GDestroyNotify) sonatina_profile_free);

	chooser_update();

	/* TODO: disconnect if this is current profile */

	return TRUE;
}

GList *sonatina_lookup_profile(const char *name)
{
	GList *cur;

	for (cur = profiles; cur; cur = cur->next) {
		if (!g_strcmp0(((struct sonatina_profile *) cur->data)->name, name)) {
			return cur;
		}
	}

	return NULL;
}

const struct sonatina_profile *sonatina_get_profile(const char *name)
{
	GList *node;

	node = sonatina_lookup_profile(name);

	if (!node) {
		return NULL;
	}

	return (struct sonatina_profile *) node->data;
}

void sonatina_profile_free(struct sonatina_profile *profile)
{
	g_assert(profile != NULL);

	if (profile->name) {
		g_free(profile->name);
	}

	if (profile->host) {
		g_free(profile->host);
	}

	if (profile->password) {
		g_free(profile->password);
	}

	g_free(profile);
}

