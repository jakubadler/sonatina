#ifndef UTIL_H
#define UTIL_H

#include <gtk/gtk.h>
#include <mpd/client.h>

enum {
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_WARNING,
	LEVEL_ERROR,
	LEVEL_FATAL
};

extern int log_level;

int printmsg(int level, const char *format, ...);

#ifdef DEBUG
#define MSG_DEBUG(...) { printmsg(LEVEL_DEBUG, __VA_ARGS__); }
#else
#define MSG_DEBUG(...) {}
#endif

#define MSG_INFO(...) { printmsg(LEVEL_INFO, __VA_ARGS__); }
#define MSG_WARNING(...) { printmsg(LEVEL_WARNING, __VA_ARGS__); }
#define MSG_ERROR(...) { printmsg(LEVEL_ERROR, __VA_ARGS__); }
#define MSG_FATAL(...) { printmsg(LEVEL_FATAL, __VA_ARGS__); }

#define INT_BUF_SIZE 24


/**
  An entity defined by MPD tags. Depending on which fields are used, this entity
  represents either a genre, an artist or an album. Functions that work with
  this structure resemble libmpd's API for entities.
  */
struct mpd_tag_entity {
	gchar *genre;
	gchar *artist;
	gchar *album;
	gchar *date;
};

struct mpd_tag_entity *mpd_tag_entity_begin(const struct mpd_pair *pair);
gboolean mpd_tag_entity_feed(struct mpd_tag_entity *entity, const struct mpd_pair *pair);
void mpd_tag_entity_free(struct mpd_tag_entity *entity);

GtkBuilder *load_tab_ui(const char *name);

/**
  @brief Begin parsing a new entity.
  @param pair First pair in this entity.
  @returns Newly allocated mpd_tag_entity.
  */
struct mpd_tag_entity *mpd_tag_entity_begin(const struct mpd_pair *pair);

/**
  @brief Parse pair and add its information to the entity.
  @param entity @a mpd_tag_entity returned by @a mpd_tag_entity_begin().
  @param pair New pair to parse.
  @returns TRUE if the pair was parsed and added to the entity (or if the pair
  was not understood and ignored), FALSE if this pair is the beginning of a next
  entity.
  */
gboolean mpd_tag_entity_feed(struct mpd_tag_entity *entity, const struct mpd_pair *pair);

/**
  @brief Free @a mpd_tag_entity allocated by @a mpd_tag_entity_begin().
  @param entity A @a mpd_tag_entity object.
  */
void mpd_tag_entity_free(struct mpd_tag_entity *entity);

#endif
