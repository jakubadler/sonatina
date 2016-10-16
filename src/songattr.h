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

gchar *song_attr_format(const char *format, const struct mpd_song *song);

#endif
