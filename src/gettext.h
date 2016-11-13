#ifndef SONATINA_GETTEXT_H
#define SONATINA_GETTEXT_H


#if ENABLE_NLS
#	include <libintl.h>
#	include <locale.h>
#	define _(x) gettext (x)
#else
#	define _(x) (x)
#endif

#endif
