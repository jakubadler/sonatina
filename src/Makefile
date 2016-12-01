include ../config.mk

SRC=	main.c core.c profile.c settings.c gui.c client.c util.c songattr.c playlist.c library.c pathbar.c
HEAD=	       core.h profile.h settings.h gui.h client.h util.h songattr.h playlist.h library.h pathbar.h
OBJ=	${SRC:.c=.o}
BIN=	${PROG}

all: ${BIN}

${BIN}: ${OBJ}
	${CC} ${OBJ} ${LDFLAGS} -o ${BIN}

${OBJ}: ${HEAD}

.c.o: ${HEAD}
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

install: ${BIN}
	install -d ${BINDIR}
	install -t ${BINDIR} ${BIN}

uninstall:
	rm -f ${BINDIR}/${BIN}

clean:
	rm -f ${OBJ}
	rm -f ${BIN}

