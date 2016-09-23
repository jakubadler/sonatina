include config.mk

build:
	make -C src
	make -C share
	make -C po

install: all
	make -C src install
	make -C share install
	make -C po install

uninstall:
	make -C src uninstall
	make -C share uninstall
	make -C po uninstall

clean:
	make -C src clean
	make -C share clean
	make -C po clean

test:
	make install
	${PREFIX}/bin/${PROG}
	make uninstall

