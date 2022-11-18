###############################################################################
#                                                                             #
# MAKEFILE for Cloak                                                          #
#                                                                             #
# (c) Guy Wilson 2022                                                         #
#                                                                             #
###############################################################################

# Version number for cloak
MAJOR_VERSION = 2
MINOR_VERSION = 1

# Directories
SOURCE = src
RESOURCE=resources
BUILD = build
DEP = dep

# What is our target
TARGET = cloak

# Tools
VBUILD = vbuild
CC = gcc
LINKER = gcc

LIBDIRS=-L/opt/homebrew/lib

ifdef GUI
GTKINCLUDES=`pkg-config --cflags gtk4`
GTKLIBRARIES=`pkg-config --libs gtk4`

DEFINES=-DBUILD_GUI

RESOURCEC=glib-compile-resources
RESTARGET=cloak-resources
RESOURCESRC=$(SOURCE)/$(RESTARGET).c
RESOURCEOBJ=$(BUILD)/$(RESTARGET).o
RESOURCEDEF=cloak.gresource.xml
RESOURCEXML=builder.ui
RESOURCEFLAGS = --target=$(RESOURCESRC) --sourcedir=. --compiler=$(CC) --generate-source 
RESOURCE.c = $(RESOURCEC) $(RESOURCEFLAGS)
endif

INCLUDEDIRS=-I/opt/homebrew/include $(GTKINCLUDES)
LIBRARIES = -lgcrypt -lpng $(GTKLIBRARIES)

PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CFLAGS = -c -O2 -Wall -pedantic $(DEFINES) $(INCLUDEDIRS)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

COMPILE.c = $(CC) $(CFLAGS) $(DEPFLAGS) -o $@
LINK.o = $(LINKER) $(LIBDIRS) -o $@

CSRCFILES = $(wildcard $(SOURCE)/*.c)
OBJFILES := $(patsubst $(SOURCE)/%.c, $(BUILD)/%.o, $(CSRCFILES))
DEPFILES = $(patsubst $(SOURCE)/%.c, $(DEP)/%.d, $(CSRCFILES))
RESFILES = $(wildcard $(RESOURCE)/*.*)

all: $(TARGET)

# Compile C/C++ source files
#
$(TARGET): $(OBJFILES) $(RESOURCEOBJ)
	$(LINK.o) $^ $(LIBRARIES)
	rm -f $(RESOURCESRC)

$(BUILD)/%.o: $(SOURCE)/%.c
$(BUILD)/%.o: $(SOURCE)/%.c $(DEP)/%.d
	$(PRECOMPILE)
	$(COMPILE.c) $<
	$(POSTCOMPILE)

$(RESOURCESRC): $(RESOURCEDEF) $(RESFILES)
	$(RESOURCE.c) $<

.PRECIOUS = $(DEP)/%.d
$(DEP)/%.d: ;

-include $(DEPFILES)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin

version:
	$(VBUILD) -incfile cloak.ver -template version.c.template -out $(SOURCE)/version.c -major $(MAJOR_VERSION) -minor $(MINOR_VERSION)

clean:
	rm -r $(BUILD)
	rm -r $(DEP)
	rm $(TARGET)
