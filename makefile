# Directories
SRCDIR		?= ./src
OBJDIR		?= ./obj
BINDIR		?= ./bin
SHAREDIR	?= ./share
PREFIX		?= /usr/local

EXECUTABLE	?= $(BINDIR)/jis-gui

SOURCES		:= $(shell find $(SRCDIR) -name '*.c')
OBJECTS		:= $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJDIRS		:= $(sort $(dir $(OBJECTS)))
DEPENDS		:= $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.d, $(SOURCES))

EXTDEPS		:= raylib
EXTCFLAGS	:= $(shell pkg-config --cflags --libs $(EXTDEPS))

# Compiler
CC		:= gcc
CFLAGS		:= -Wall -Werror -Isrc/

.PHONY: debug build		\
	clean gen-bear		\

# Compiling profiles
all: build

debug: CFLAGS += -g
debug: CPPFLAGS +=
debug: $(OBJDIRS) $(EXECUTABLE)

build: CFLAGS += -O3
build: CPPFLAGS += -DNDEBUG
build: $(OBJDIRS) $(EXECUTABLE)

# Header dependencies
-include $(DEPENDS)

$(BINDIR):
	mkdir $(BINDIR)

$(EXECUTABLE): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) $(EXTCFLAGS) $(OBJECTS) $(OBJLIBS) -o $@

$(OBJDIRS):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c makefile
	$(CC) $(CFLAGS) $(EXTCFLAGS) -MMD -MP -c $< -o $@

install: $(EXECUTABLE)
	install -Dm 755 $(BINDIR)/jis-gui $(DESTDIR)$(PREFIX)/bin/jis-gui
	for path in $$(find $(SHAREDIR) -type f); do \
		install -Dm 744 $$path "$(DESTDIR)$(PREFIX)/$$path"; \
	done

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/jis-gui
	rm -rf $(DESTDIR)$(PREFIX)/share/jis-gui/

# Miscellaneous
clean:
	rm -rf $(OBJDIR) $(BINDIR)

gen-bear: clean
	bear -- make
