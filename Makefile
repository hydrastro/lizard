# Lizard — top-level build
#
# Layout:
#   include/lizard.h     public API header
#   src/*.{c,h}          implementation + private headers
#   tests/*.c            C-level unit tests
#   tests/*.lisp         end-to-end interpreter tests (golden output)
#   examples/*.lisp      example programs
#   docs/*.md            roadmap and API sketch
#
# Targets:
#   make                 build lib + repl (release mode)
#   make test            build & run all C tests + golden-output tests
#   make examples        run every example file
#   make install         install lib + headers + binary under PREFIX
#   make clean           remove build artefacts
#   make distclean       deep clean
#
# Development modes (Phase 0 of LIZARD_EVOLUTION_PLAN):
#   make MODE=debug      O0 + full debug info
#   make MODE=asan test  AddressSanitizer + UB sanitizer
#   make MODE=coverage   gcov-instrumented (use via 'make coverage')
#   make MODE=release    O2 (default)
#
# Tooling:
#   make debug           shortcut for MODE=debug all
#   make asan            shortcut for MODE=asan test
#   make coverage        rebuild with coverage, run tests, write gcov reports
#   make profile         release build + perf record on benchmark
#   make doctor          run build/runtime/repository diagnostics
#   make lint-tree       fail if generated or misplaced files are present
#   make format          clang-format -i across src/ and tests/

CC       ?= gcc
AR       ?= ar
INSTALL  ?= install
RM       ?= rm -f
RMDIR    ?= rm -rf
PREFIX   ?= /usr/local
MODE     ?= release

BUILD_DIR   := build
SRC_DIR     := src
INC_DIR     := include
TEST_DIR    := tests
EXAMPLE_DIR := examples
LIB_DIR     := lib
RUN_ENV     ?=

# --- flags ---------------------------------------------------------------
CPPFLAGS ?=
override CPPFLAGS += -I$(INC_DIR) -I$(SRC_DIR)

WARNINGS := -Wall -Wextra -Werror -pedantic -pedantic-errors
WARNINGS += -Waggregate-return -Wbad-function-cast -Wcast-align -Wcast-qual
WARNINGS += -Wdeclaration-after-statement -Wfloat-equal -Wlogical-op
WARNINGS += -Wmissing-declarations -Wmissing-include-dirs -Wmissing-prototypes
WARNINGS += -Wnested-externs -Wpointer-arith -Wredundant-decls
WARNINGS += -Wsequence-point -Wstrict-prototypes -Wswitch -Wundef
WARNINGS += -Wunreachable-code -Wwrite-strings -Wconversion
WARNINGS += -Wno-unused-parameter

CFLAGS ?=
CFLAGS += -std=c89 -fPIC $(WARNINGS) -fno-common
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security

LDFLAGS ?=
LDLIBS  ?= -lds -lgmp

# Mode-specific tweaks.
ifeq ($(MODE),debug)
  CFLAGS += -O0 -g3
else ifeq ($(MODE),asan)
  CFLAGS += -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
  LDFLAGS += -fsanitize=address,undefined
else ifeq ($(MODE),coverage)
  CFLAGS += -O0 -g3 --coverage
  LDFLAGS += --coverage
else
  CFLAGS += -O2
endif

# --- sources -------------------------------------------------------------
LIB_SRCS := runtime lizard env mem parser primitives prims_common prims_lists prims_modules prims_gc prims_syntax prims_persistent prims_collections prims_tt prims_bytecode tokenizer printer tt_equality tt_registry tt_check gc bytecode kernel tactics
LIB_OBJS := $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(LIB_SRCS)))

# --- artifacts -----------------------------------------------------------
LIB_STATIC := $(BUILD_DIR)/liblizard.a
LIB_SHARED := $(BUILD_DIR)/liblizard.so
REPL_BIN   := $(BUILD_DIR)/lizard
REPL_OBJ   := $(BUILD_DIR)/repl.o

.PHONY: all check ci clean distclean doctor install uninstall lint-tree test examples \
        debug asan coverage profile format smoke help

all: $(LIB_STATIC) $(LIB_SHARED) $(REPL_BIN)
check: test examples
ci: lint-tree test examples smoke

debug:
	$(MAKE) MODE=debug all

asan:
	$(MAKE) MODE=asan test

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

# Include auto-generated dependency files so header changes trigger
# rebuilds. -MMD writes a .d file alongside each .o describing which
# headers the .c file depends on; -MP adds phony targets for each
# header so removing a header doesn't break the build.
-include $(LIB_OBJS:.o=.d)
-include $(REPL_OBJ:.o=.d)

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
	@$(RUN_ENV) ./scripts/run-examples.sh $(REPL_BIN)

# --- profiling / coverage -----------------------------------------------
smoke: $(REPL_BIN)
	@$(RUN_ENV) $(REPL_BIN) --eval '(+ 40 2)'

profile: MODE=release
profile: $(REPL_BIN)
	@command -v perf >/dev/null 2>&1 || { echo "perf not found"; exit 1; }
	perf record --call-graph=dwarf -- $(RUN_ENV) $(REPL_BIN) < $(EXAMPLE_DIR)/22-benchmarks.lisp >/dev/null
	perf report

coverage:
	$(MAKE) clean
	$(MAKE) MODE=coverage test
	@mkdir -p coverage
	@gcov -o $(BUILD_DIR) $(SRC_DIR)/*.c >/dev/null || true
	@mv *.gcov coverage/ 2>/dev/null || true
	@echo "Coverage reports written to coverage/*.gcov"

doctor:
	@./scripts/dev.sh doctor

lint-tree:
	@./scripts/clean.sh --check

# --- install -------------------------------------------------------------
install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/include
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/share/lizard
	$(INSTALL) -m 0644 $(LIB_STATIC) $(DESTDIR)$(PREFIX)/lib/
	$(INSTALL) -m 0755 $(LIB_SHARED) $(DESTDIR)$(PREFIX)/lib/
	$(INSTALL) -m 0755 $(REPL_BIN)   $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -m 0644 $(INC_DIR)/lizard.h $(DESTDIR)$(PREFIX)/include/
	$(INSTALL) -m 0644 $(INC_DIR)/lizard_api.h $(DESTDIR)$(PREFIX)/include/
	$(INSTALL) -m 0644 $(LIB_DIR)/prelude.lisp $(DESTDIR)$(PREFIX)/share/lizard/

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/lib/liblizard.a
	$(RM) $(DESTDIR)$(PREFIX)/lib/liblizard.so
	$(RM) $(DESTDIR)$(PREFIX)/bin/lizard
	$(RM) $(DESTDIR)$(PREFIX)/include/lizard.h
	$(RM) $(DESTDIR)$(PREFIX)/include/lizard_api.h
	$(RM) $(DESTDIR)$(PREFIX)/share/lizard/prelude.lisp

# --- format --------------------------------------------------------------
format:
	@command -v clang-format >/dev/null || { echo "clang-format not found"; exit 1; }
	clang-format -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(INC_DIR)/*.h $(TEST_DIR)/*.c

clean:
	$(RMDIR) $(BUILD_DIR) coverage
	$(RM) *.gcov *.gcda *.gcno

distclean: clean
	@./scripts/clean.sh

help:
	@echo "Targets:    all test examples check ci smoke doctor lint-tree clean distclean install uninstall format"
	@echo "Modes:      make MODE=debug | make MODE=asan test | make MODE=coverage"
	@echo "Tooling:    make debug | make asan | make coverage | make profile"
	@echo "Runtime:    set RUN_ENV='LD_LIBRARY_PATH=/path/to/ds/lib' when needed"
