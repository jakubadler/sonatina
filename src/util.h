#ifndef UTIL_H
#define UTIL_H

enum {
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_WARNING,
	LEVEL_ERROR,
	LEVEL_FATAL
};

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

#endif
