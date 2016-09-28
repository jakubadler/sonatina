include ../config.mk

SRC=	main.c core.c settings.c gui.c client.c util.c
HEAD=	       core.h settings.h gui.h client.h util.h
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
	mkdir -p ${PREFIX}/bin
	cp -f ${BIN} ${PREFIX}/bin/${BIN}

uninstall:
	rm -f ${PREFIX}/bin/${BIN}

clean:
	rm -f ${OBJ}
	rm -f ${BIN}

