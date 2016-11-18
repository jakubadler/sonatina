#include <glib.h>
#include <mpd/client.h>

#include "songattr.h"
#include "gettext.h"

int song_attr_format_to_type(char c)
{
	switch (c) {
	case 'A':
		return MPD_TAG_ARTIST;
	case 'B':
		return MPD_TAG_ALBUM;
	case 'T':
		return MPD_TAG_TITLE;
	case 'N':
		return MPD_TAG_TRACK;
	case 'D':
		return MPD_TAG_DISC;
/*	case 'Y':
		return MPD_TAG_YEAR;*/
	case 'G':
		return MPD_TAG_GENRE;
	case 'P':
		return SONG_ATTR_URI;
	case 'F':
		return SONG_ATTR_FILE;
	case 'L':
		return SONG_ATTR_LENGTH;
	case 'E':
		return SONG_ATTR_TIME;
	default:
		break;
	}
	return -1;
}

gchar *get_song_untagged(enum mpd_tag_type attr, const struct mpd_song *song)
{
	g_assert(song != NULL);

	switch (attr) {
	case MPD_TAG_TITLE:
		return get_song_attr(SONG_ATTR_FILE, song);
	case MPD_TAG_ARTIST:
	case MPD_TAG_ALBUM:
	default:
		return g_strdup(_("Untagged"));
	}

	return NULL;
}

gchar *get_song_attr(enum song_attr attr, const struct mpd_song *song)
{
	int num = 0;
	gchar *result;
	const char *tag;

	g_assert(song != NULL);

	if (attr < (enum song_attr) MPD_TAG_COUNT) {
		tag = mpd_song_get_tag(song, attr, 0);
		if (tag) {
			result = g_strdup(tag);
		} else {
			result = get_song_untagged(attr, song);
		}
		return result;
	}

	switch (attr) {
	case SONG_ATTR_ID:
		num = mpd_song_get_id(song);
		result = g_strdup_printf("%d", num);
		break;
	case SONG_ATTR_POS:
		num = mpd_song_get_pos(song);
		result = g_strdup_printf("%d", num);
		break;
	case SONG_ATTR_LENGTH:
		num = mpd_song_get_duration(song);
		result = g_strdup_printf("%d:%.2d", num/60, num%60);
		break;
	case SONG_ATTR_TIME:
		result = g_strdup_printf("%d:%.2d", num/60, num%60);
		break;
	case SONG_ATTR_MOD:
		num = mpd_song_get_last_modified(song);
		/* TODO: proper time format */
		result = g_strdup_printf("%d", num);
		break;
	case SONG_ATTR_URI:
		result = g_strdup(mpd_song_get_uri(song));
		break;
	case SONG_ATTR_FILE:
		result = g_path_get_basename(mpd_song_get_uri(song));
		break;
	default:
		result = g_strdup(_("(unknown)"));
		break;
	}

	return result;
}

const char *get_song_attr_name(enum song_attr attr)
{
	switch (attr) {
	case SONG_ATTR_ID:
		return _("ID");
	case SONG_ATTR_POS:
		return _("Position");
	case SONG_ATTR_LENGTH:
		return _("Length");
	case SONG_ATTR_URI:
		return _("Path");
	case SONG_ATTR_FILE:
		return _("Filename");
	case SONG_ATTR_MOD:
		return _("Modification time");
	case SONG_ATTR_TIME:
		return _("Elapsed time");
	default:
		break;
	}

	if (attr < (enum song_attr) MPD_TAG_COUNT) {
		return mpd_tag_name(attr);
	}

	return NULL;
}

gchar *song_attr_format(const char *format, const struct mpd_song *song)
{
	GString *buf;
	enum song_attr type;
	char *attr;
	const char *name;
	size_t i;
	int block = 0;

	buf = g_string_new("");

	for (i = 0; format[i]; i++) {
		if (format[i] == '%') {
			i++;
			type = song_attr_format_to_type(format[i]);
			if (song) {
				attr = get_song_attr(type, song);
				g_string_append(buf, attr);
				g_free(attr);
			} else {
				name = get_song_attr_name(type);
				g_string_append(buf, name);
			}
		} else if (format[i] == '{') {
			block++;
		} else if (format[i] == '}') {
			block--;
		} else {
			g_string_append_c(buf, format[i]);
		}
	}
	
	return g_string_free(buf, FALSE);

}

