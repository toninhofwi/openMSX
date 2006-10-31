# $Id$
#
# Replacement for "configure".
# Performs some test compiles, to check for headers and functions.
# Unlike configure, it does not run any test code, so it is more friendly for
# cross compiles.

# This Makefile needs parameters to operate; check that they were specified:
# - output directory
ifeq ($(OUTDIR),)
$(error Missing parameter: OUTDIR)
endif
# - command line to invoke compiler + options
ifeq ($(COMPILE),)
$(error Missing parameter: COMPILE)
endif
# - OS we're building for
ifeq ($(OPENMSX_TARGET_OS),)
$(error Missing parameter: OPENMSX_TARGET_OS)
endif

# Result files.
LOG:=$(OUTDIR)/probe.log
OUTHEADER:=$(OUTDIR)/probed_defs.hh
OUTMAKE:=$(OUTDIR)/probed_defs.mk


# Functions
# =========

ALL_FUNCS:=FTRUNCATE GETTIMEOFDAY MMAP USLEEP POSIX_MEMALIGN

FTRUNCATE_FUNC:=ftruncate
FTRUNCATE_HEADER:=<unistd.h>

GETTIMEOFDAY_FUNC:=gettimeofday
GETTIMEOFDAY_HEADER:=<sys/time.h>

MMAP_FUNC:=mmap
MMAP_HEADER:=<sys/mman.h>

USLEEP_FUNC:=usleep
USLEEP_HEADER:=<unistd.h>

POSIX_MEMALIGN_FUNC:=posix_memalign
POSIX_MEMALIGN_HEADER:=<stdlib.h>

# Disabled X11, because it is not useful yet and the link flags are not here.
#X11_FUNC:=XtMalloc
#X11_HEADER:=<X11/Intrinsic.h>


# Headers
# =======

ALL_HEADERS:=$(addsuffix _H, \
	GL GL_GL GLEW GL_GLEW JACK PNG SDL SDL_IMAGE SYS_MMAN SYS_SOCKET TCL XML \
	ZLIB )

# Location of GL headers is not standardised; if one of these matches,
# we consider the GL headers found.
GL_HEADER:=<gl.h>
GL_GL_HEADER:=<GL/gl.h>
# Use GL_CFLAGS for GL_GL as well, if someone overrides it.
# In any case, only GL_CFLAGS will be used by the actual build.
GL_GL_CFLAGS=$(GL_CFLAGS)

# The comment for the GL headers applies to GLEW as well.
GLEW_HEADER:=<glew.h>
GL_GLEW_HEADER:=<GL/glew.h>
GL_GLEW_CFLAGS=$(GLEW_CFLAGS)

JACK_HEADER:=<jack/jack.h>

PNG_HEADER:=<png.h>
PNG_CFLAGS:=`libpng-config --cflags 2>> $(LOG)`

SDL_HEADER:=<SDL.h>
SDL_CFLAGS:=`sdl-config --cflags 2>> $(LOG)`

SDL_IMAGE_HEADER:=<SDL_image.h>
# Note: "=" instead of ":=", so overriden value of SDL_CFLAGS will be used.
SDL_IMAGE_CFLAGS=$(SDL_CFLAGS)

SYS_MMAN_HEADER:=<sys/mman.h>

SYS_SOCKET_HEADER:=<sys/socket.h>

TCL_HEADER:=<tcl.h>
TCL_CFLAGS:=`build/tcl-search.sh --cflags 2>> $(LOG)`

XML_HEADER:=<libxml/parser.h>
XML_CFLAGS:=`xml2-config --cflags 2>> $(LOG)`

ZLIB_HEADER:=<zlib.h>


# Libraries
# =========

ALL_LIBS:=GL GLEW JACK PNG SDL SDL_IMAGE TCL XML ZLIB
ALL_LIBS+=ABC XYZ

GL_LDFLAGS:=-lGL
GL_RESULT:=yes

GLEW_LDFLAGS:=-lGLEW
GLEW_RESULT:=yes

JACK_LDFLAGS:=-ljack
JACK_RESULT:=yes

PNG_LDFLAGS:=`libpng-config --ldflags 2>> $(LOG)`
PNG_RESULT:=`libpng-config --version`

SDL_LDFLAGS:=`sdl-config --libs 2>> $(LOG)`
SDL_RESULT:=`sdl-config --version`

# Note: "=" instead of ":=", so overriden value of SDL_LDFLAGS will be used.
SDL_IMAGE_LDFLAGS=$(SDL_LDFLAGS) -lSDL_image
SDL_IMAGE_RESULT:=yes

TCL_LDFLAGS:=`build/tcl-search.sh --ldflags 2>> $(LOG)`
TCL_RESULT:=`build/tcl-search.sh --version 2>> $(LOG)`

XML_LDFLAGS:=`xml2-config --libs 2>> $(LOG)`
XML_RESULT:=`xml2-config --version`

ZLIB_LDFLAGS:=-lz
ZLIB_RESULT:=yes

# Libraries that do not exist:

ABC_LDFLAGS:=`abc-config --libs 2>> $(LOG)`
ABC_RESULT:=impossible

XYZ_LDFLAGS:=-lxyz
XYZ_RESULT:=impossible


# OS Specific
# ===========

DISABLED_FUNCS:=
DISABLED_LIBS:=
DISABLED_HEADERS:=

# Allow the OS specific Makefile to override if necessary.
include build/platform-$(OPENMSX_TARGET_OS).mk


# Implementation
# ==============

CHECK_FUNCS:=$(filter-out $(DISABLED_FUNCS),$(ALL_FUNCS))
CHECK_HEADERS:=$(filter-out $(DISABLED_HEADERS),$(ALL_HEADERS))
CHECK_LIBS:=$(filter-out $(DISABLED_LIBS),$(ALL_LIBS))

CHECK_TARGETS:=hello $(ALL_FUNCS) $(ALL_HEADERS) $(ALL_LIBS)
PRINT_LIBS:=$(addsuffix -print,$(CHECK_LIBS))

.PHONY: all init check-targets print-libs $(CHECK_TARGETS) $(PRINT_LIBS)

# Default target.
all: check-targets print-libs
check-targets: $(CHECK_TARGETS)
print-libs: $(PRINT_LIBS)

# Create empty log and result files.
init:
	@echo "Probing target system..."
	@mkdir -p $(OUTDIR)
	@echo "Probing system:" > $(LOG)
	@echo "// Automatically generated by build system." > $(OUTHEADER)
	@echo "# Automatically generated by build system." > $(OUTMAKE)
	@echo "# Non-empty value means found, empty means not found." >> $(OUTMAKE)
	@echo "PROBE_MAKE_INCLUDED:=true" >> $(OUTMAKE)
	@echo "DISABLED_FUNCS:=$(DISABLED_FUNCS)" >> $(OUTMAKE)
	@echo "DISABLED_LIBS:=$(DISABLED_LIBS)" >> $(OUTMAKE)
	@echo "DISABLED_HEADERS:=$(DISABLED_HEADERS)" >> $(OUTMAKE)
	@echo "HAVE_X11:=" >> $(OUTMAKE)

# Check compiler with the most famous program.
hello: init
	@echo "#include <iostream>" > $(OUTDIR)/$@.cc
	@echo "int main(int argc, char** argv) {" >> $(OUTDIR)/$@.cc
	@echo "  std::cout << \"Hello World!\" << std::endl;" >> $(OUTDIR)/$@.cc
	@echo "}" >> $(OUTDIR)/$@.cc
	@if $(COMPILE) $(CXXFLAGS) -c $(OUTDIR)/$@.cc -o $(OUTDIR)/$@.o 2>> $(LOG); \
	then echo "Compiler works: $(COMPILE) $(CXXFLAGS)" >> $(LOG); \
	     echo "COMPILER:=true" >> $(OUTMAKE); \
	else echo "Compiler broken: $(COMPILE) $(CXXFLAGS)" >> $(LOG); \
	     echo "COMPILER:=false" >> $(OUTMAKE); \
	fi
	@rm -f $(OUTDIR)/$@.cc $(OUTDIR)/$@.o

# Probe for function:
# Try to include the necessary header and get the function address.
$(CHECK_FUNCS): init
	@echo > $(OUTDIR)/$@.cc
	@if [ -n "$($@_PREHEADER)" ]; then echo "#include $($@_PREHEADER)"; fi \
		>> $(OUTDIR)/$@.cc
	@echo "#include $($@_HEADER)" >> $(OUTDIR)/$@.cc
	@echo "void (*f)() = reinterpret_cast<void (*)()>($($@_FUNC));" \
		>> $(OUTDIR)/$@.cc
	@if $(COMPILE) $(CXXFLAGS) -c $(OUTDIR)/$@.cc -o $(OUTDIR)/$@.o \
		2>> $(LOG); \
	then echo "Found function: $@" >> $(LOG); \
	     echo "#define HAVE_$@ 1" >> $(OUTHEADER); \
	     echo "HAVE_$@:=true" >> $(OUTMAKE); \
	else echo "Missing function: $@" >> $(LOG); \
	     echo "// #undef HAVE_$@" >> $(OUTHEADER); \
	     echo "HAVE_$@:=" >> $(OUTMAKE); \
	fi
	@rm -f $(OUTDIR)/$@.cc $(OUTDIR)/$@.o

$(DISABLED_FUNCS): init
	@echo "Disabled function: $@" >> $(LOG)
	@echo "// #undef HAVE_$@" >> $(OUTHEADER)
	@echo "HAVE_$@:=" >> $(OUTMAKE)

# Probe for header:
# Try to include the header.
$(CHECK_HEADERS): init
	@echo > $(OUTDIR)/$@.cc
	@if [ -n "$($(@:%_H=%)_PREHEADER)" ]; then \
		echo "#include $($(@:%_H=%)_PREHEADER)"; fi >> $(OUTDIR)/$@.cc
	@echo "#include $($(@:%_H=%)_HEADER)" >> $(OUTDIR)/$@.cc
	@if FLAGS="$($(@:%_H=%_CFLAGS))" && $(COMPILE) $(CXXFLAGS) $$FLAGS \
		-c $(OUTDIR)/$@.cc -o $(OUTDIR)/$@.o 2>> $(LOG); \
	then echo "Found header: $(@:%_H=%)" >> $(LOG); \
	     echo "#define HAVE_$@ 1" >> $(OUTHEADER); \
	     echo "HAVE_$@:=true" >> $(OUTMAKE); \
	else echo "Missing header: $(@:%_H=%)" >> $(LOG); \
	     echo "// #undef HAVE_$@" >> $(OUTHEADER); \
	     echo "HAVE_$@:=" >> $(OUTMAKE); \
	fi
	@rm -f $(OUTDIR)/$@.cc $(OUTDIR)/$@.o

$(DISABLED_HEADERS): init
	@echo "Disabled header: $(@:%_H=%)" >> $(LOG)
	@echo "// #undef HAVE_$@" >> $(OUTHEADER)
	@echo "HAVE_$@:=" >> $(OUTMAKE)

# Probe for library:
# Try to link dummy program to the library.
$(CHECK_LIBS): init
	@echo "int main(int argc, char **argv) { return 0; }" > $(OUTDIR)/$@.cc
	@if FLAGS="$($@_LDFLAGS)" && $(COMPILE) $(CXXFLAGS) \
		$(OUTDIR)/$@.cc -o $(OUTDIR)/$@.exe $(LINK_FLAGS) $$FLAGS 2>> $(LOG); \
	then echo "Found library: $@" >> $(LOG); \
	     echo "#define HAVE_$@_LIB 1" >> $(OUTHEADER); \
	     echo "HAVE_$@_LIB:=$($@_RESULT)" >> $(OUTMAKE); \
	else echo "Missing library: $@" >> $(LOG); \
	     echo "// #undef HAVE_$@_LIB" >> $(OUTHEADER); \
	     echo "HAVE_$@_LIB:=" >> $(OUTMAKE); \
	fi
	@rm -f $(OUTDIR)/$@.cc $(OUTDIR)/$@.exe

$(DISABLED_LIBS): init
	@echo "Disabled library: $@" >> $(LOG)
	@echo "// #undef HAVE_$@_LIB" >> $(OUTHEADER)
	@echo "HAVE_$@_LIB:=" >> $(OUTMAKE)

# Print the flags for using a certain library (CFLAGS and LDFLAGS).
$(PRINT_LIBS): check-targets
	@echo "$(@:%-print=%)_CFLAGS:=$($(@:%-print=%)_CFLAGS)" >> $(OUTMAKE)
	@echo "$(@:%-print=%)_LDFLAGS:=$($(@:%-print=%)_LDFLAGS)" >> $(OUTMAKE)

