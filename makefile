### Building process.

# Compiler, libs and flags.
CC ?= gcc
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0 polkit-gobject-1)
LDFLAGS = $(shell $(PKGCONFIG) --libs gtk+-3.0 polkit-gobject-1)
FLAGS = -g -O0 -Wall -pthread -pipe
# Source and objects.
SRC := $(wildcard *.c)
OBJS := $(SRC:.c=.o)

all: gdm-background
gdm-background: $(OBJS)
	@echo "Linking objects, libraries, etc...."
	$(CC) $^ -o $@ $(LDFLAGS)
%.o: %.c
	@echo "Compiling and Creating object...."
	$(CC) $(FLAGS) -c $^ -o $@ $(CFLAGS)
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) gdm-background

### Installing process.

# Dbus service and main program.
PREFIX := /usr/share
POLKIT := $(PREFIX)/polkit-1/actions/
SERVICE := $(PREFIX)/dbus-1/system-services/
CONFIG := $(PREFIX)/dbus-1/system.d/
SERVER := /usr/libexec/
SHORTCUT := $(PREFIX)/applications/
PROGRAM := /usr/local/bin/

install: gdm-background
	install -m 644 xyz.thiggy01.GDMBackground.policy $(POLKIT)
	install -m 644 xyz.thiggy01.GDMBackground.service $(SERVICE)
	install -m 644 xyz.thiggy01.GDMBackground.conf $(CONFIG)
	install -m 755 gdm-background-helper $(SERVER)
	install -m 644 gdm-background.desktop $(SHORTCUT)
	install -m 755 gdm-background $(PROGRAM)

uninstall: gdm-background
	rm $(POLKIT)/xyz.thiggy01.GDMBackground.policy
	rm $(SERVICE)/xyz.thiggy01.GDMBackground.service
	rm $(CONFIG)/xyz.thiggy01.GDMBackground.conf
	rm $(SERVER)/gdm-background-helper
	rm $(SHORTCUT)/gdm-background.desktop
	rm $(PROGRAM)/gdm-background
