PREFIX=		${HOME}
PROG=		sonatina
LIBS=		libmpdclient gtk+-3.0
CFLAGS=		-std=c99 -D_DEFAULT_SOURCE \
		-Wall -Wextra -pedantic -Wno-unused-parameter \
		-DDEBUG -g \
		-DPROG=\"${PROG}\" \
		-DPREFIX=\"${PREFIX}\" \
		-DSHAREDIR=\"${PREFIX}/share/${PROG}\" \
		$(shell pkg-config --cflags ${LIBS})
LDFLAGS=	$(shell pkg-config --libs ${LIBS})

.PHONY: build install uninstall clean

all: build
