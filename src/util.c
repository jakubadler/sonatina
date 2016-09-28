#include <stdio.h>
#include <stdarg.h>

#include "util.h"

#ifdef DEBUG
int log_level = LEVEL_DEBUG;
#else
int log_level = LEVEL_WARNING;
#endif

int printmsg(int level, const char *format, ...)
{
	va_list ap;

	if (level < log_level) {
		return 0;
	}

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);

	return 0;
}

