include config.mk

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

clean:
	make -C src clean
	make -C data clean
	make -C po clean

test:
	make install
	${BINDIR}/${PROG}
	make uninstall

