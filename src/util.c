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
	FILE *out;
	va_list ap;

	out = stderr;

	if (level < log_level) {
		return 0;
	}

	va_start(ap, format);
	switch (level) {
	case LEVEL_DEBUG:
		fprintf(out, "DEBUG: ");
		break;
	case LEVEL_INFO:
		fprintf(out, "INFO: ");
		break;
	case LEVEL_WARNING:
		fprintf(out, "WARNING: ");
		break;
	case LEVEL_ERROR:
		fprintf(out, "ERROR: ");
		break;
	case LEVEL_FATAL:
	default:
		fprintf(out, "FATAL: ");
		break;
	}
	vfprintf(out, format, ap);
	fprintf(out, "\n");
	va_end(ap);

	return 0;
}

