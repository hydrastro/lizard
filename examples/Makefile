# Lizard — top-level build
#
# Layout:
#   include/lizard.h     public API header
#   src/*.{c,h}          implementation + private headers
#   tests/*.c            C-level unit tests
#   tests/*.lisp         end-to-end interpreter tests (golden output)
#   examples/*.lisp      example programs
#
# Targets:
#   make            build lib + repl
#   make test       build & run all C tests + golden-output tests
#   make examples   run every example file
#   make install    install lib + headers + binary under PREFIX
#   make clean      remove build artefacts

CC       ?= gcc
AR       ?= ar
INSTALL  ?= install
PREFIX   ?= /usr/local

BUILD_DIR  := build
SRC_DIR    := src
INC_DIR    := include
TEST_DIR   := tests
EXAMPLE_DIR := examples

# --- flags ---------------------------------------------------------------
CFLAGS   ?= -O2 -fPIC
CFLAGS   += -std=c89 -Wall -Wextra -Werror -pedantic -pedantic-errors
CFLAGS   += -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual
CFLAGS   += -Wdeclaration-after-statement -Wfloat-equal -Wlogical-op
CFLAGS   += -Wmissing-declarations -Wmissing-include-dirs -Wmissing-prototypes
CFLAGS   += -Wnested-externs -Wpointer-arith -Wredundant-decls
CFLAGS   += -Wsequence-point -Wstrict-prototypes -Wswitch -Wundef
CFLAGS   += -Wunreachable-code -Wwrite-strings -Wconversion
CFLAGS   += -Wno-unused-parameter
CFLAGS   += -fno-common -fstack-protector-strong
CFLAGS   += -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security

CPPFLAGS += -I$(INC_DIR) -I$(SRC_DIR)
LDLIBS   ?= -lds -lgmp

# --- sources -------------------------------------------------------------
LIB_SRCS := lizard env mem parser primitives tokenizer printer tt_equality tt_check
LIB_OBJS := $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(LIB_SRCS)))

# --- artifacts -----------------------------------------------------------
LIB_STATIC := $(BUILD_DIR)/liblizard.a
LIB_SHARED := $(BUILD_DIR)/liblizard.so
REPL_BIN   := $(BUILD_DIR)/lizard
REPL_OBJ   := $(BUILD_DIR)/repl.o

.PHONY: all clean distclean install uninstall test examples format
all: $(LIB_STATIC) $(LIB_SHARED) $(REPL_BIN)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB_STATIC): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(LIB_SHARED): $(LIB_OBJS)
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(REPL_BIN): $(REPL_OBJ) $(LIB_STATIC)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# --- tests ---------------------------------------------------------------
include tests/tests.mk

# --- examples ------------------------------------------------------------
examples: $(REPL_BIN)
	@set -e; for f in $(EXAMPLE_DIR)/*.lisp; do \
	  echo "=== $$f ==="; \
	  $(REPL_BIN) < "$$f" || exit $$?; \
	done

# --- install -------------------------------------------------------------
install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/include
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -m 0644 $(LIB_STATIC) $(DESTDIR)$(PREFIX)/lib/
	$(INSTALL) -m 0755 $(LIB_SHARED) $(DESTDIR)$(PREFIX)/lib/
	$(INSTALL) -m 0755 $(REPL_BIN)   $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 0644 $(INC_DIR)/lizard.h $(DESTDIR)$(PREFIX)/include/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/liblizard.a
	rm -f $(DESTDIR)$(PREFIX)/lib/liblizard.so
	rm -f $(DESTDIR)$(PREFIX)/bin/lizard
	rm -f $(DESTDIR)$(PREFIX)/include/lizard.h

# --- format --------------------------------------------------------------
format:
	@command -v clang-format >/dev/null || { echo "clang-format not found"; exit 1; }
	clang-format -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(INC_DIR)/*.h $(TEST_DIR)/*.c

clean:
	rm -rf $(BUILD_DIR)

# Deeper cleanup — also wipes profiler output, editor junk, core files.
# Pass --deep to additionally remove the Nix result symlink and flake.lock.
distclean:
	@./scripts/clean.sh
