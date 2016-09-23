include ../config.mk

install: ${PROG}.desktop
	mkdir -p ${PREFIX}/share/applications
	cp -f ${PROG}.desktop ${PREFIX}/share/applications/${PROG}.desktop
	mkdir -p ${PREFIX}/share/${PROG}
	cp -f ${PROG}.ui ${PREFIX}/share/${PROG}/${PROG}.ui

uninstall:
	rm -f ${PREFIX}/share/applications/${PROG}.desktop
	rm -rf ${PREFIX}/share/${PROG}
