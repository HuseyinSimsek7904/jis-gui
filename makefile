# Directories
SRCDIR		?= ./src
OBJDIR		?= ./obj
BINDIR		?= ./bin

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

# Binary files
$(BINDIR):
	mkdir $(BINDIR)

$(EXECUTABLE): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) $(EXTCFLAGS) $(OBJECTS) $(OBJLIBS) -o $@

# Object files
$(OBJDIRS):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c makefile
	$(CC) $(CFLAGS) $(EXTCFLAGS) -MMD -MP -c $< -o $@

# Miscellaneous
clean:
	rm -rf $(OBJDIR) $(BINDIR)

gen-bear: clean
	bear -- make
