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
C = gcc
LINKER = gcc

# postcompile step
PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
# postcompile step
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CFLAGS = -c -O2 -Wall -pedantic -I/opt/homebrew/include -I/Users/guy/Library/include `pkg-config --cflags gtk4`
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

# Libraries
STDLIBS = 
EXTLIBS = -lgcrypt -lpng `pkg-config --libs gtk4`

COMPILE.c = $(C) $(CFLAGS) $(DEPFLAGS) -o $@
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
