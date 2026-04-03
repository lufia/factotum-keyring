<$PLAN9/src/mkhdr

TARG=factotum-keyring

CFLAGS=-I/usr/include/dbus-1.0 -I/lib64/dbus-1.0/include
LDFLAGS=-ldbus-1

HFILES=

OFILES=\
	main.$O\

<$PLAN9/src/mkone
