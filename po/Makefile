include ../config.mk

.PHONY: all clean install uninstall

PO=$(wildcard *.po)
MO=${PO:.po=.mo}
LL=${PO:%.po=%}

all: ${MO}

%.mo: %.po
	msgfmt -o $@ $<

clean:
	rm -f ${MO}

install: ${MO}
	$(foreach ll,${LL}, \
		install -d ${LOCALEDIR}/${ll}/LC_MESSAGES; \
		install ${ll}.mo ${LOCALEDIR}/${ll}/LC_MESSAGES/${PACKAGE}.mo \
	)

uninstall:
	rm -f ${LOCALEDIR}/*/LC_MESSAGES/${PACKAGE}.mo

