baseutils
=========
`baseutils` is a collection of Unix userland utilities from OpenBSD.

It is designed to be configuration-less; just run `make` to build
everything and `sudo make install` to install everything.

You can customize the installation directory by setting the `PREFIX`
environment variable during `sudo make install`, for example:
`sudo make PREFIX=/usr install`.

You can customize the manual page installation directory by setting
the `MANDIR` environment variable during `sudo make install`, for
example:
`sudo make MANDIR=/usr/share/man install`.

Some installation recommendations:
* `sudo make PREFIX=/ MANDIR=/usr/share/man install` for global installation
* `sudo make PREFIX=/usr MANDIR=/usr/share/man install` for another type of global installation
* `sudo make PREFIX=/usr/local MANDIR=/usr/local/share/man` for package installation
* `sudo make PREFIX=/opt MANDIR=/opt/share/man` for pkgsrc installation
* `sudo make PREFIX=/usr/ucb MANDIR=/usr/ucb/share/man` for Sun-style *BSD utility installation

Don't forget to add $PREFIX/bin to your $PATH in order to use these
utilities!

Building
--------
You will need BSD make or GNU make, or another make that understands
the `-C` flag the same way those two do.

You will need clang or gcc.

`baseutils` will use its own tools (yacc, lex, m4, mv, etc.) during
build, thus no external Unix utilities are necessary besides make
and a compiler toolchain.

`baseutils` assumes C99 and a POSIX interface.

Testing
-------
`baseutils` is built on Void Linux for glibc testing and Alpine
Linux for musl-libc compatibility.

Mac OS X, FreeBSD, NetBSD, and DragonFly BSD testing appreciated.

Testing on other operating systems appreciated as well.

Licensing
---------
These utilities carry BSD-style licenses.

libz carries a zlib license.

Utilities
---------
* `[(1)`
* `apply(1)`
* `arch(1)`
* `awk(1)`
* `banner(1)`
* `basename(1)`
* `bc(1)` (from NetBSD)
* `biff(1)`
* `cat(1)`
* `ci(1)`
* `cmp(1)`
* `co(1)`
* `col(1)`
* `colrm(1)`
* `column(1)`
* `comm(1)`
* `compress(1)`
* `cp(1)`
* `cpio(1)`
* `csplit(1)`
* `ctags(1)`
* `cut(1)`
* `date(1)`
* `dd(1)`
* `deroff(1)`
* `diff(1)`
* `dirname(1)`
* `echo(1)`
* `ed(1)`
* `egrep(1)`
* `env(1)`
* `expand(1)`
* `expr(1)`
* `false(1)`
* `fgrep(1)`
* `flex(1)`
* `flex++(1)`
* `fmt(1)`
* `fold(1)`
* `grep(1)`
* `gunzip(1)`
* `gzcat(1)`
* `gzexe(1)`
* `gzip(1)`
* `ident(1)`
* `indent(1)`
* `join(1)`
* `jot(1)`
* `lex(1)`
* `ln(1)`
* `ls(1)`
* `m4(1)`
* `machine(1)`
* `merge(1)`
* `mkdir(1)`
* `mv(1)`
* `nl(1)`
* `opencvs(1)`
* `paste(1)`
* `patch(1)`
* `pax(1)`
* `printf(1)`
* `pwd(1)`
* `rcs(1)`
* `rcsclean(1)`
* `rcsdiff(1)`
* `rcsmerge(1)`
* `rlog(1)`
* `rm(1)`
* `rmdir(1)`
* `sed(1)`
* `sleep(1)`
* `sync(8)`
* `tar(1)`
* `tee(1)`
* `test(1)`
* `true(1)`
* `uname(1)`
* `uncompress(1)`
* `unifdef(1)`
* `uniq(1)`
* `wc(1)`
* `who(1)`
* `whois(1)`
* `xargs(1)`
* `yacc(1)`
* `yes(1)`
* `yyfix(1)`
* `zcat(1)`
* `zcmp(1)`
* `zdiff(1)`
* `zegrep(1)`
* `zfgrep(1)`
* `zforce(1)`
* `zgrep(1)`
* `zless(1)`
* `zmore(1)`
* `znew(1)`
