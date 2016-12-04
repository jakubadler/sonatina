#ifndef SONATINA_PROFILE_H
#define SONATINA_PROFILE_H

struct sonatina_profile {
	gchar *name;
	gchar *host;
	gint port;
	gchar *password;
};

extern GList *profiles; /** List of loaded profiles (struct sonatina_profile) */

/**
  @brief Load profiles from file. Each successful call to @a
  sonatina_profiles_load() must be matched with a call to @a
  sonatina_profiles_save().
  @returns TRUE on success, FALSE on error.
  */
gboolean sonatina_profiles_load();

/**
  @brief Save loaded profiles to file. Must only be called after a successful
  call to @a sonatina_profiles_load().
  @returns TRUE on success, FALSE on error.
  */
gboolean sonatina_profiles_save();

/**
  @brief Create new connection profile.
  @param name Name of the new profile.
  @param host Hostname.
  @param port TCP port.
  @param password Optional password.
  */
void sonatina_add_profile(const char *name, const char *host, int port, const char *password);

/**
  @brief Modify a loaded profile specified by its name.
  @param name Name of the profile to be modified.
  @param newname New name of the profile or NULL if name is not to be changed.
  @param host New host parameter of the profile or NULL if host is not to be changed.
  @param port New port parameter of the profile or NULL if port is not to be changed.
  @param port New password parameter of the profile or NULL if password is not to be changed.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_modify_profile(const char *name, const char *newname, const char *host, int port, const char *password);

/**
  @brief Delete a profile specified by its name.
  @param name Name of the profile to be removed.
  @returns TRUE on success, FALSE otherwise.
  */
gboolean sonatina_remove_profile(const char *name);

/**
  @brief Lookup a profile in the profile list. Profile can be modified throught
  the returned pointer.
  @param name Name of the profile.
  @returns GList item in the list of loaded profiles or NULL if requested
  profile doesn't exist.
  */
GList *sonatina_lookup_profile(const char *name);

/**
  @brief Get profile by its name.
  @param name Name of the profile.
  @returns Profile with given name or NULL if not found.
  */
const struct sonatina_profile *sonatina_get_profile(const char *name);

/**
  @brief Free profile structure and its members.
  */
void sonatina_profile_free(struct sonatina_profile *profile);

#endif
