NetBSD bc
=========
NetBSD bc is a BSD-licensed `bc(1)` implementation that comes with the NetBSD
Operating System.

Supported systems
-----------------
NetBSD bc should run on any minimally `POSIX` operating system. Pull requests
are much appreciated to support more operating systems.

Supported compilers
-------------------
NetBSD bc is known to build with the following C compilers:
* clang (https://llvm.org/)
* gcc (https://gcc.gnu.org/)
* pcc (http://pcc.ludd.ltu.se/)
* cparser (https://pp.ipd.kit.edu/firm/)

Building with a compiler not listed here? Add it and send a pull request!

Dependencies
------------
Any C89 compiler should be able to compile NetBSD bc. Please see the list of C
compilers above for a list of known working compilers.

Additionally, `libedit` and `libreadline` are checked to see if they are
available. If one is, NetBSD bc will link with it for command-line editing. If
neither are present, NetBSD bc will still compile, but will not have
command-line editing support.

Compiling
---------
```
$ ./configure
$ make
$ sudo make install
```

License
-------
BSD. See individual file headers for more details.
