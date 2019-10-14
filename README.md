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

Don't forget to add $PREFIX/bin to your $PATH in order to use these utilities!

Building
--------
You will need BSD make or GNU make, or another make that understands
the `-C` flag the same way those two do.

You will need clang or gcc, or a C99 compiler that understands the
`-include` flag the same way those two do.

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
* `cat(1)`
* `compress(1)`
* `cp(1)`
* `cpio(1)`
* `date(1)`
* `dd(1)`
* `echo(1)`
* `ed(1)`
* `expr(1)`
* `gunzip(1)`
* `gzcat(1)`
* `gzexe(1)`
* `gzip(1)`
* `ln(1)`
* `ls(1)`
* `machine(1)`
* `mkdir(1)`
* `mv(1)`
* `pax(1)`
* `pwd(1)`
* `rm(1)`
* `rmdir(1)`
* `sleep(1)`
* `sync(8)`
* `tar(1)`
* `test(1)`
* `uname(1)`
* `uncompress(1)`
* `zcat(1)`
* `zcmp(1)`
* `zdiff(1)`
* `zforce(1)`
* `zless(1)`
* `zmore(1)`
* `znew(1)`
