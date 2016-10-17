#ifndef SONGATTR_H
#define SONGATTR_H

enum song_attr {
	SONG_ATTR_ID = MPD_TAG_COUNT,
	SONG_ATTR_POS,
	SONG_ATTR_LENGTH,
	SONG_ATTR_URI,
	SONG_ATTR_FILE,
	SONG_ATTR_MOD,
	SONG_ATTR_TIME,
	SONG_ATTR_COUNT
};

/**
  @brief Get value of an attribute from song.
  @param attr Attribute
  @param song Song
  @returns Newly allocated string that should be freed with g_free().
  */
gchar *get_song_attr(int attr, const struct mpd_song *song);

/**
  @brief Get attribute name.
  @param attr Attribute
  @returns Attribute name.
  */
const char *get_song_attr_name(int attr);

/**
  @brief Replace certain '%'-specifiers with song attributes.
  @param format Format string.
  @param song mpdclient song whose attributes should be used. If @a song is
  NULL, attribute names will be used instead of values.
  @return Newly allocated string that should be freed with g_free().
  */
gchar *song_attr_format(const char *format, const struct mpd_song *song);

#endif
