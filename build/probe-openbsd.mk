# $Id$

MMAP_PREHEADER:=<sys/types.h>
SYS_MMAN_PREHEADER:=<sys/types.h>
SYS_SOCKET_PREHEADER:=<sys/types.h>

# TODO: tcl-search.sh should provide a proper value for CFLAGS, so do we really
#       need this?
TCL_CFLAGS_SYS_DYN:=-I/usr/local/include/tcl8.4
