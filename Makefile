include config.mk

.PHONY: all build install uninstall doc clean test update-po

all: build

build:
	make -C src
	make -C data
	make -C po

install: all
	make -C src install
	make -C data install
	make -C po install

uninstall:
	make -C src uninstall
	make -C data uninstall
	make -C po uninstall

doc:
	doxygen

clean-doc:
	rm -rf doc

clean:
	make -C src clean
	make -C data clean
	make -C po clean

update-po:
	xgettext -d ${PACKAGE} -k_ -k__ -p po --language=C src/*.c
	xgettext -d ${PACKAGE} -k_ -k__ -p po -j data/*.ui
	mv po/${PACKAGE}.po po/${PACKAGE}.pot

test:
	make install
	${BINDIR}/${PROG}
	make uninstall

