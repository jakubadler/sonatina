PREFIX?=	/usr/local
EXEC_PREFIX=	${PREFIX}
BINDIR=		${EXEC_PREFIX}/bin
SBINDIR=	${EXEC_PREFIX}/sbin
LIBEXECDIR=	${EXEC_PREFIX}/libexec
DATAROOTDIR=	${PREFIX}/share
DATADIR=	${DATAROOTDIR}
SYSCONFDIR=	${PREFIX}/etc
PROG=		sonatina
LIBS=		libmpdclient gtk+-3.0
CFLAGS=		-std=c99 -D_DEFAULT_SOURCE \
		-Wall -Wextra -pedantic -Wno-unused-parameter \
		-DDEBUG -g \
		-DPROG=\"${PROG}\" \
		-DDATADIR=\"${DATADIR}\" \
		-DSYSCONFDIR=\"${SYSCONFDIR}\" \
		$(shell pkg-config --cflags ${LIBS})
LDFLAGS=	$(shell pkg-config --libs ${LIBS})

.PHONY: build install uninstall clean

all: build
