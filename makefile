###############################################################################
#                                                                             #
# MAKEFILE for Cloak                                                          #
#                                                                             #
# (c) Guy Wilson 2022                                                         #
#                                                                             #
###############################################################################

# Version number for cloak
MAJOR_VERSION = 2
MINOR_VERSION = 0

# Directories
SOURCE = src
BUILD = build
DEP = dep

# What is our target
TARGET = cloak

# Tools
VBUILD = vbuild
CC = gcc
LINKER = gcc
RESOURCEC=glib-compile-resources

RESOURCETARGET=$(SOURCE)/cloak-resources.c
RESOURCEDEF=cloak.gresource.xml
RESOURCEXML=builder.ui

# postcompile step
PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
# postcompile step
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CFLAGS = -c -O2 -Wall -pedantic -I/opt/homebrew/include -I/Users/guy/Library/include `pkg-config --cflags gtk4`
RESOURCEFLAGS = --target=$@ --sourcedir=. --compiler=$(CC) --generate-source 
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

# Libraries
STDLIBS = 
EXTLIBS = -lgcrypt -lpng `pkg-config --libs gtk4`

COMPILE.c = $(CC) $(CFLAGS) $(DEPFLAGS) -o $@
RESOURCE.c = $(RESOURCEC) $(RESOURCEFLAGS)
LINK.o = $(LINKER) $(STDLIBS) -L/opt/homebrew/lib -L/Users/guy/Library/lib -o $@

CSRCFILES = $(wildcard $(SOURCE)/*.c)
OBJFILES := $(patsubst $(SOURCE)/%.c, $(BUILD)/%.o, $(CSRCFILES))
DEPFILES = $(patsubst $(SOURCE)/%.c, $(DEP)/%.d, $(CSRCFILES))

all: $(TARGET)

# Compile C/C++ source files
#
$(TARGET): $(OBJFILES)
	$(LINK.o) $^ $(EXTLIBS)

$(BUILD)/%.o: $(SOURCE)/%.c
$(BUILD)/%.o: $(SOURCE)/%.c $(DEP)/%.d
	$(PRECOMPILE)
	$(COMPILE.c) $<
	$(POSTCOMPILE)

$(RESOURCETARGET): $(RESOURCEDEF) $(RESOURCEXML) initialimage.png
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
