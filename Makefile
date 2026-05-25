CC ?= gcc
AR ?= ar
INSTALL ?= install

PREFIX ?= /usr/local
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
BINDIR ?= $(PREFIX)/bin

CPPFLAGS ?=
CFLAGS ?= -std=c89 -O2 -fPIC \
  -Wall -Wextra -Werror -pedantic -pedantic-errors \
  -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual \
  -Wdeclaration-after-statement -Wfloat-equal -Wlogical-op \
  -Wmissing-declarations -Wmissing-include-dirs -Wmissing-prototypes \
  -Wnested-externs -Wpointer-arith -Wredundant-decls -Wsequence-point \
  -Wstrict-prototypes -Wswitch -Wundef -Wunreachable-code \
  -Wwrite-strings -Wconversion -Wno-unused-parameter \
  -fno-common -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
  -Wformat -Wformat-security
LDFLAGS ?=
LDLIBS ?= -lds -lgmp -pthread
ARFLAGS ?= rcs

LIB_SRCS := lizard.c env.c mem.c parser.c primitives.c tokenizer.c printer.c
LIB_OBJS := $(LIB_SRCS:.c=.o)
REPL_SRCS := repl.c
REPL_OBJS := $(REPL_SRCS:.c=.o)

.PHONY: all clean install uninstall
all: liblizard.a liblizard.so lizard

liblizard.a: $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

liblizard.so: $(LIB_OBJS)
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

lizard: $(REPL_OBJS) liblizard.a
	$(CC) $(LDFLAGS) -o $@ $(REPL_OBJS) liblizard.a $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

install: all
	$(INSTALL) -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR)/lizard $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0644 liblizard.a $(DESTDIR)$(LIBDIR)/
	$(INSTALL) -m 0755 liblizard.so $(DESTDIR)$(LIBDIR)/
	$(INSTALL) -m 0755 lizard $(DESTDIR)$(BINDIR)/
	$(INSTALL) -m 0644 *.h $(DESTDIR)$(INCLUDEDIR)/lizard/
	$(INSTALL) -m 0644 lizard.h $(DESTDIR)$(INCLUDEDIR)/

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/liblizard.a $(DESTDIR)$(LIBDIR)/liblizard.so
	rm -f $(DESTDIR)$(BINDIR)/lizard
	rm -rf $(DESTDIR)$(INCLUDEDIR)/lizard
	rm -f $(DESTDIR)$(INCLUDEDIR)/lizard.h

clean:
	rm -f $(LIB_OBJS) $(REPL_OBJS) liblizard.a liblizard.so lizard
