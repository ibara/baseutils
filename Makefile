# baseutils top Makefile

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

all:
	make -C libopenbsd
	make -C libz
	make -C apply
	make -C arch
	make -C cat
	make -C compress
	make -C cp
	make -C date
	make -C dd
	make -C echo
	make -C ed
	make -C expr
	make -C ln
	make -C ls
	make -C mkdir
	make -C mv
	make -C pax
	make -C pwd
	make -C rm
	make -C rmdir
	make -C sleep
	make -C sync
	make -C test
	make -C uname

install:
	install -d -m 755 ${PREFIX}/bin
	install -d -m 755 ${MANDIR}/man1
	install -d -m 755 ${MANDIR}/man7
	install -d -m 755 ${MANDIR}/man8
	make -C apply install
	make -C arch install
	make -C cat install
	make -C compress install
	make -C cp install
	make -C date install
	make -C dd install
	make -C echo install
	make -C ed install
	make -C expr install
	make -C ln install
	make -C ls install
	make -C mkdir install
	make -C mv install
	make -C pax install
	make -C pwd install
	make -C rm install
	make -C rmdir install
	make -C sleep install
	make -C sync install
	make -C test install
	make -C uname install

clean:
	make -C libopenbsd clean
	make -C libz clean
	make -C apply clean
	make -C arch clean
	make -C cat clean
	make -C compress clean
	make -C cp clean
	make -C date clean
	make -C dd clean
	make -C echo clean
	make -C ed clean
	make -C expr clean
	make -C ln clean
	make -C ls clean
	make -C mkdir clean
	make -C mv clean
	make -C pax clean
	make -C pwd clean
	make -C rm clean
	make -C rmdir clean
	make -C sleep clean
	make -C sync clean
	make -C test clean
	make -C uname clean
