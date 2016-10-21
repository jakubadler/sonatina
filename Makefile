include config.mk

.PHONY: build install uninstall doc clean test

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

test:
	make install
	${BINDIR}/${PROG}
	make uninstall

