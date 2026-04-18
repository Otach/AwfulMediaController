CC = gcc
CFLAGS = -Wall -Wextra -O2 -Wno-unused-parameter -lm
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

TESTDIR = tests
TEST_CFLAGS = -Wall -Wextra -g -I./awfulmc $(shell $(PKGCONFIG) --cflags $(LIBRARIES))
TEST_LDFLAGS = -lcheck -pthread -lrt -lm $(shell $(PKGCONFIG) --libs $(LIBRARIES))

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -o $@ $^ $(LIB_FLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(LIB_CFLAGS) -MMD -c $< -o $@

-include $(DEP)

test: $(TESTDIR)/bin/test_amcqueue
	@echo "Running Tests..."
	@$(TESTDIR)/bin/test_amcqueue

$(TESTDIR)/bin/test_amcqueue: awfulmc/amc_queue.c tests/test_amcqueue.c
	@mkdir -p $(TESTDIR)/bin
	$(CC) $(TEST_CFLAGS) -o $@ tests/test_amcqueue.c awfulmc/amc_queue.c $(TEST_LDFLAGS)

clean:
	rm -rf $(BUILDDIR)
	rm -f $(BINDIR)/awfulmc
	rm -rf $(TESTDIR)/bin

install: $(TARGET)
	@mkdir -p $(BINDIR)
	install -m 0755 $(TARGET) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/awfulmc

.PHONY: all clean install uninstall
