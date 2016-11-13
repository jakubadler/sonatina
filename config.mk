PREFIX?=	/usr/local
EXEC_PREFIX=	${PREFIX}
BINDIR=		${EXEC_PREFIX}/bin
SBINDIR=	${EXEC_PREFIX}/sbin
LIBEXECDIR=	${EXEC_PREFIX}/libexec
DATAROOTDIR=	${PREFIX}/share
DATADIR=	${DATAROOTDIR}
SYSCONFDIR=	${PREFIX}/etc
LOCALEDIR=	${DATAROOTDIR}/locale
DEBUG?=		-O2
PACKAGE=	sonatina
PROG=		${PACKAGE}


ENABLE_NLS?=	1

LIBS=		libmpdclient gtk+-3.0
CFLAGS=		-std=c99 -D_DEFAULT_SOURCE \
		-Wall -Wextra -pedantic -Wno-unused-parameter -Wno-missing-field-initializers \
		${DEBUG} \
		-DPROG=\"${PROG}\" \
		-DPACKAGE=\"${PROG}\" \
		-DLOCALEDIR=\"${LOCALEDIR}\" \
		-DDATADIR=\"${DATADIR}\" \
		-DSYSCONFDIR=\"${SYSCONFDIR}\" \
		-DENABLE_NLS=${ENABLE_NLS} \
		$(shell pkg-config --cflags ${LIBS})
LDFLAGS=	$(shell pkg-config --libs ${LIBS})

.PHONY: build install uninstall clean

all: build
