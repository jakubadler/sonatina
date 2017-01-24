include ../config.mk

install: ${PROG}.desktop
	install -d ${DATADIR}/applications
	install -t ${DATADIR}/applications ${PROG}.desktop
	install -d ${DATADIR}/${PROG}
	install -t ${DATADIR}/${PROG} *.ui
	cp -r -t ${DATADIR} icons


uninstall:
	rm -f ${DATADIR}/applications/${PROG}.desktop
	rm -rf ${DATADIR}/${PROG}
	rm -rf ${DATADIR}/icons/hicolor/*/apps/sonatina.png
	rm -rf ${DATADIR}/icons/hicolor/*/apps/sonatina.svg
