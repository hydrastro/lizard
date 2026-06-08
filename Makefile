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
LDLIBS  ?= -lds -lgmp -lm

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
# Keep the core order explicit, but close over any additional implementation
# modules present in src/.  This prevents scaffold/test modules from compiling
# against headers whose .c file was accidentally left out of liblizard.a.
LIB_CORE_SRCS := runtime lizard env mem parser primitives tokenizer printer tt_equality tt_check gc bytecode kernel tactics
LIB_OPTIONAL_SRCS := prims_tt tt_check_modal tt_check_hit tt_check_fresh tt_check_cubical tt_logic tt_faces prims_syntax prims_bytecode prims_gc prims_lists prims_modules prims_logic prims_collections prims_persistent prims_string prims_kernel diagnostics object_model gc_metadata hamt pvector lzrt inet ic ic_lower kt_to_core id_observe net_eval report_writer report_schema diagnostic_report expansion_trace_report syntax_expansion_report surface_term expansion_context syntax_expander core_term kernel_sexp tt_glue tt_lattice elaborator
EXISTING_OPTIONAL_LIB_SRCS := $(foreach m,$(LIB_OPTIONAL_SRCS),$(if $(wildcard $(SRC_DIR)/$(m).c),$(m)))
LIB_SRCS := $(LIB_CORE_SRCS) $(filter-out $(LIB_CORE_SRCS),$(EXISTING_OPTIONAL_LIB_SRCS))
LIB_OBJS := $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(LIB_SRCS)))

# --- artifacts -----------------------------------------------------------
LIB_STATIC := $(BUILD_DIR)/liblizard.a
LIB_SHARED := $(BUILD_DIR)/liblizard.so
REPL_BIN   := $(BUILD_DIR)/lizard
REPL_OBJ   := $(BUILD_DIR)/repl.o

.PHONY: all check ci clean distclean doctor install uninstall lint-tree test examples examples-audit api-audit header-audit include-audit ownership-audit build-graph-audit \
        debug asan coverage profile format smoke help

all: $(LIB_STATIC) $(LIB_SHARED) $(REPL_BIN)
check: test examples
ci: lint-tree api-audit header-audit include-audit ownership-audit build-graph-audit test examples smoke

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

# Phase 13b: cross-check the interaction net against the trusted kernel kt_whnf.
# Builds the differential test against liblizard.a (which provides the kernel,
# the heap, and the net) and runs it.  Override the count: make ic-kernel-diff N=50000
N ?= 20000
.PHONY: ic-kernel-diff
ic-kernel-diff: $(LIB_STATIC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(SRC_DIR) -Iinclude \
	    tests/ic_kernel_diff_test.c $(LIB_STATIC) $(LDLIBS) -lgmp -o $(BUILD_DIR)/ic_kernel_diff_test
	$(BUILD_DIR)/ic_kernel_diff_test $(N) 1

# Recursion-as-cycles: a recursive definition is a self-referential graph (a
# `ref D a` node whose unfolding contains further `ref D ...` nodes), unfolded
# lazily and terminating because the definition branches on its now-concrete
# argument.  Drives fact/sumto/fib/pow2/gcd through the net (incl. a bignum 25!
# on the GMP path) and checks against C oracles.
.PHONY: ic-recursion
ic-recursion:
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(SRC_DIR) -Iinclude \
	    tests/ic_recursion_test.c $(SRC_DIR)/ic.c -lgmp -o $(BUILD_DIR)/ic_recursion_test
	$(BUILD_DIR)/ic_recursion_test

# Phase 14c: the by-observation identity reduction system and its executable
# specification.  Id_A(x,y) computes by recursion on the structure of A (Bool/Nat
# structurally, product componentwise, function pointwise/funext, universe by
# univalence, transport-refl as identity).  No kernel oracle exists for these
# rules, so the checks are against hand-computed normal forms.
.PHONY: id-observe
id-observe:
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(SRC_DIR) \
	    tests/id_observe_test.c $(SRC_DIR)/id_observe.c -o $(BUILD_DIR)/id_observe_test
	$(BUILD_DIR)/id_observe_test

# Phase 16 (started): the net as evaluator.  Runs closed kernel terms on the net
# and reads results back into kterms (kt_eval_via_net), behind a gate with a
# kt_whnf fallback (kt_normalize_gated); checked against the trusted reducer.
.PHONY: net-eval
net-eval: $(LIB_STATIC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(SRC_DIR) -Iinclude \
	    tests/net_eval_test.c $(LIB_STATIC) $(LDLIBS) -lgmp -o $(BUILD_DIR)/net_eval_test
	$(BUILD_DIR)/net_eval_test

$(LIB_SHARED): $(LIB_OBJS)
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(REPL_BIN): $(REPL_OBJ) $(LIB_STATIC)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# --- tests ---------------------------------------------------------------
include tests/tests.mk

# --- examples ------------------------------------------------------------
examples: $(REPL_BIN)
	@$(RUN_ENV) ./scripts/run-examples.sh $(REPL_BIN)

examples-audit:
	@./scripts/check-example-manifest.sh --suggest

api-audit:
	@./scripts/check-public-api.sh

header-audit:
	@./scripts/check-header-boundaries.sh

include-audit:
	@./scripts/check-include-layers.py

ownership-audit:
	@./scripts/check-ownership-audit.py

build-graph-audit:
	@./scripts/check-build-graph.py

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
	@echo "Targets:    all test examples examples-audit api-audit header-audit include-audit ownership-audit check ci smoke doctor lint-tree clean distclean install uninstall format"
	@echo "Modes:      make MODE=debug | make MODE=asan test | make MODE=coverage"
	@echo "Tooling:    make debug | make asan | make coverage | make profile"
	@echo "Runtime:    set RUN_ENV='LD_LIBRARY_PATH=/path/to/ds/lib' when needed"
