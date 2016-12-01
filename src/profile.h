#ifndef SONATINA_PROFILE_H
#define SONATINA_PROFILE_H

struct sonatina_profile {
	gchar *name;
	gchar *host;
	gint port;
	gchar *password;
};

gboolean sonatina_profiles_load();

/**
  @brief Create new connection profile.
  @param name Name of the new profile.
  @param host Hostname.
  @param port TCP port.
  @param password Optional password.
  */
void sonatina_add_profile(const char *name, const char *host, int port, const char *password);

gboolean sonatina_modify_profile(const char *name, const struct sonatina_profile *new);
gboolean sonatina_remove_profile(const char *name);

GList *sonatina_lookup_profile(const char *name);

/**
  @brief Get connection profile by name.
  @param name Name of the profile.
  @returns Profile with given name or NULL if not found.
  */
const struct sonatina_profile *sonatina_get_profile(const char *name);

/**
  @brief Free profile structure and its members.
  */
void sonatina_profile_free(struct sonatina_profile *profile);

#endif
