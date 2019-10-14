# baseutils top Makefile

PREFIX ?=	/usr/local
MANDIR ?=	/usr/local/share/man

all:
	make -C libopenbsd
	make -C libz
	make -C yacc
	make -C mv
	make -C m4
	make -C lex
	make -C sed
	make -C apply
	make -C arch
	make -C awk
	make -C banner
	make -C basename
	make -C bc
	make -C biff
	make -C cal
	make -C cat
	make -C cmp
	make -C col
	make -C colrm
	make -C column
	make -C comm
	make -C compress
	make -C cp
	make -C csplit
	make -C ctags
	make -C cut
	make -C cvs
	make -C date
	make -C dd
	make -C deroff
	make -C diff
	make -C dirname
	make -C du
	make -C echo
	make -C ed
	make -C env
	make -C expand
	make -C expr
	make -C false
	make -C fmt
	make -C fold
	make -C grep
	make -C indent
	make -C join
	make -C jot
	make -C ln
	make -C ls
	make -C mkdir
	make -C nl
	make -C paste
	make -C patch
	make -C pax
	make -C printf
	make -C pwd
	make -C rcs
	make -C rm
	make -C rmdir
	make -C sleep
	make -C sync
	make -C tee
	make -C test
	make -C true
	make -C uname
	make -C unifdef
	make -C uniq
	make -C wc
	make -C who
	make -C whois
	make -C xargs
	make -C yes

install:
	install -d -m 755 ${PREFIX}/bin
	install -d -m 755 ${MANDIR}/man1
	install -d -m 755 ${MANDIR}/man5
	install -d -m 755 ${MANDIR}/man7
	install -d -m 755 ${MANDIR}/man8
	make -C apply install
	make -C arch install
	make -C awk install
	make -C banner install
	make -C basename install
	make -C bc install
	make -C biff install
	make -C cal install
	make -C cat install
	make -C cmp install
	make -C col install
	make -C colrm install
	make -C column install
	make -C comm install
	make -C compress install
	make -C cp install
	make -C csplit install
	make -C ctags install
	make -C cut install
	make -C cvs install
	make -C date install
	make -C dd install
	make -C deroff install
	make -C diff install
	make -C dirname install
	make -C du install
	make -C echo install
	make -C ed install
	make -C env install
	make -C expand install
	make -C expr install
	make -C false install
	make -C fmt install
	make -C fold install
	make -C grep install
	make -C indent install
	make -C join install
	make -C jot install
	make -C lex install
	make -C ln install
	make -C ls install
	make -C m4 install
	make -C mkdir install
	make -C mv install
	make -C nl install
	make -C paste install
	make -C patch install
	make -C pax install
	make -C printf install
	make -C pwd install
	make -C rcs install
	make -C rm install
	make -C rmdir install
	make -C sed install
	make -C sleep install
	make -C sync install
	make -C tee install
	make -C test install
	make -C true install
	make -C uname install
	make -C unifdef install
	make -C uniq install
	make -C wc install
	make -C who install
	make -C whois install
	make -C xargs install
	make -C yacc install
	make -C yes install

clean:
	make -C libopenbsd clean
	make -C libz clean
	make -C apply clean
	make -C arch clean
	make -C awk clean
	make -C banner clean
	make -C basename clean
	make -C bc clean
	make -C biff clean
	make -C cal clean
	make -C cat clean
	make -C cmp clean
	make -C col clean
	make -C colrm clean
	make -C column clean
	make -C comm clean
	make -C compress clean
	make -C cp clean
	make -C csplit clean
	make -C ctags clean
	make -C cut clean
	make -C cvs clean
	make -C date clean
	make -C dd clean
	make -C deroff clean
	make -C diff clean
	make -C dirname clean
	make -C du clean
	make -C echo clean
	make -C ed clean
	make -C env clean
	make -C expand clean
	make -C expr clean
	make -C false clean
	make -C fmt clean
	make -C fold clean
	make -C grep clean
	make -C indent clean
	make -C join clean
	make -C jot clean
	make -C lex clean
	make -C ln clean
	make -C ls clean
	make -C m4 clean
	make -C mkdir clean
	make -C mv clean
	make -C nl clean
	make -C paste clean
	make -C patch clean
	make -C pax clean
	make -C printf clean
	make -C pwd clean
	make -C rcs clean
	make -C rm clean
	make -C rmdir clean
	make -C sed clean
	make -C sleep clean
	make -C sync clean
	make -C tee clean
	make -C test clean
	make -C true clean
	make -C uname clean
	make -C unifdef clean
	make -C uniq clean
	make -C wc clean
	make -C who clean
	make -C whois clean
	make -C xargs clean
	make -C yacc clean
	make -C yes clean
