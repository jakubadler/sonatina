include ../config.mk

install: ${PROG}.desktop
	mkdir -p ${DATADIR}/applications
	cp -f ${PROG}.desktop ${DATADIR}/applications/${PROG}.desktop
	mkdir -p ${DATADIR}/${PROG}
	cp -f ${PROG}.ui ${DATADIR}/${PROG}/${PROG}.ui

uninstall:
	rm -f ${DATADIR}/applications/${PROG}.desktop
	rm -rf ${DATADIR}/${PROG}
