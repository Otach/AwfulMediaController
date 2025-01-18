CC = gcc
CFLAGS = -Wall -Wextra -O2 -Wno-unused-parameter
PKGCONFIG = pkg-config
LIBRARIES = gio-2.0 glib-2.0 x11 xinerama pangocairo cairo
LIB_CFLAGS = $(shell $(PKGCONFIG) --cflags $(LIBRARIES))
LIB_FLAGS = $(shell $(PKGCONFIG) --libs $(LIBRARIES))
SRCDIR = awfulmc
BUILDDIR = build
TARGET = $(BUILDDIR)/awfulmc
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

SRC = $(shell find $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRC))
DEP = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -o $@ $^ $(LIB_FLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -MMD -c $< -o $@

-include $(DEP)

clean:
	rm -rf $(BUILDDIR)

install: $(TARGET)
	@mkdir -p $(BINDIR)
	install -m 0755 $(TARGET) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/awfulmc

.PHONY: all clean install uninstall
