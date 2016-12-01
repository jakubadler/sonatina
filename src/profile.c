#include <glib.h>

#include "profile.h"

#include "util.h"
#include "core.h"
#include "gui.h"


gboolean sonatina_profiles_load()
{
	gchar *profilesfile;
	GKeyFile *profiles;
	gchar **profnames;
	size_t i;
	struct sonatina_profile *profile = NULL;
	gboolean success;
	
	profilesfile = g_build_filename(g_get_user_config_dir(), PACKAGE, "profiles.ini", NULL);
	profiles = g_key_file_new();
	success = g_key_file_load_from_file(profiles, profilesfile,
			G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
			NULL);
	g_free(profilesfile);

	if (!success) {
		return FALSE;
	}

	profnames = g_key_file_get_groups(profiles, NULL);
	sonatina.profiles = NULL;
	for (i = 0; profnames[i]; i++) {
		if (!profile) {
			profile = g_malloc(sizeof(struct sonatina_profile));
		}
		profile->name = g_strdup(profnames[i]);
		profile->host = g_key_file_get_string(profiles, profnames[i], "host", NULL);
		profile->port = g_key_file_get_integer(profiles, profnames[i], "port", NULL);
		profile->password = g_key_file_get_string(profiles, profnames[i], "password", NULL);
		if (profile->host) {
			sonatina.profiles = g_list_append(sonatina.profiles, profile);
			chooser_add_profile(profile->name);
			profile = NULL;
		}
	}

	g_key_file_free(profiles);
	g_strfreev(profnames);

	return TRUE;

}

gboolean sonatina_profiles_save()
{
	gchar *profilesfile;
	GKeyFile *profiles;
	GList *node;
	struct sonatina_profile *profile;
	gboolean success;
	
	profiles = g_key_file_new();

	for (node = sonatina.profiles; node; node = node -> next) {
		profile = (struct sonatina_profile *) node->data;
		g_key_file_set_string(profiles, profile->name, "host", profile->host);
		g_key_file_set_integer(profiles, profile->name, "port", profile->port);

		if (profile->password) {
			g_key_file_set_string(profiles, profile->password, "password", profile->password);
		}
	}

	profilesfile = g_build_filename(g_get_user_config_dir(), PACKAGE, "profiles.ini", NULL);

	success = g_key_file_save_to_file(profiles, profilesfile, NULL);

	g_free(profilesfile);
	g_key_file_free(profiles);

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

	sonatina.profiles = g_list_append(sonatina.profiles, profile);
	chooser_add_profile(profile->name);
}

gboolean sonatina_modify_profile(const char *name, const struct sonatina_profile *new)
{
	GList *node;
	struct sonatina_profile *profile;

	node = sonatina_lookup_profile(name);

	if (!node) {
		MSG_INFO("profile '%s' doesn't exist", name);
		return FALSE;
	}

	profile = (struct sonatina_profile *) node->data;

	if (new->name) {
		g_free(profile->name);
		profile->name = g_strdup(new->name);
	}

	if (new->host) {
		g_free(profile->host);
		profile->host = g_strdup(new->host);
	}

	if (new->port >= 0) {
		profile->port = new->port;
	}

	if (new->password) {
		if (profile->password) {
			g_free(profile->password);
		}
		profile->password = g_strdup(new->password);
	}

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

	sonatina.profiles = g_list_remove_link(sonatina.profiles, node);
	g_list_free_full(node, (GDestroyNotify) sonatina_profile_free);

	chooser_remove_profile(name);

	/* TODO: disconnect if this is current profile */

	return TRUE;
}

GList *sonatina_lookup_profile(const char *name)
{
	GList *cur;

	for (cur = sonatina.profiles; cur; cur = cur->next) {
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

