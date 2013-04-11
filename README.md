Intelligence: Skull Defense
===========================

The Viscount of Shelterestingstone is really in a jam, and his only way out is
to defend himself using a precious artifact.  Kill bad guys, collect cash, and
charge the skull!

Compiling
---------

To build, first set the following variables in `default/Makefile` accordingly:

    KERNEL_DIR     location of Uzebox kernel sources
    UZEBIN_DIR     location of compiled Uzebox utilities

Once this is done, the game should compile by running `make` from the
`default/` directory.

ISD was developed with avr-gcc 4.7.2 and the latest Uzebox kernel from SVN (at
the time of writing, r255), but it may work with different versions of the
aforementioned software.
