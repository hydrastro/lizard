# tests/tests.mk — included from top-level Makefile
#
# We have two flavours of tests:
#   - C unit tests (tests/*_test.c) that link against liblizard.a
#     and exercise the public API directly.
#   - Lisp end-to-end tests (tests/*.lisp + tests/*.expected) that
#     run a .lisp file through the REPL and diff against an expected
#     output file.

TEST_C_SRCS    := $(wildcard $(TEST_DIR)/*_test.c)
TEST_C_BINS    := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/tests/%,$(TEST_C_SRCS))
TEST_HELPER_OBJ := $(BUILD_DIR)/tests/test_helpers.o

TEST_LISP_SRCS := $(wildcard $(TEST_DIR)/*.lisp)

$(BUILD_DIR)/tests:
	@mkdir -p $@

$(TEST_HELPER_OBJ): $(TEST_DIR)/test_helpers.c | $(BUILD_DIR)/tests
	$(CC) $(CPPFLAGS) -I$(TEST_DIR) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%: $(TEST_DIR)/%.c $(TEST_HELPER_OBJ) $(LIB_STATIC) | $(BUILD_DIR)/tests
	$(CC) $(CPPFLAGS) -I$(TEST_DIR) $(CFLAGS) $(LDFLAGS) -o $@ $< $(TEST_HELPER_OBJ) $(LIB_STATIC) $(LDLIBS)

# Run a single Lisp golden test: <stem>.lisp + <stem>.expected.
.PHONY: test test-c test-lisp
test: test-c test-lisp
	@echo "All tests passed."

test-c: $(TEST_C_BINS)
	@set -e; failures=0; total=0; \
	for t in $(TEST_C_BINS); do \
	  total=$$((total+1)); \
	  if $(RUN_ENV) "$$t" >/dev/null 2>&1; then \
	    printf "  \033[32mPASS\033[0m  %s\n" "$$(basename $$t)"; \
	  else \
	    printf "  \033[31mFAIL\033[0m  %s\n" "$$(basename $$t)"; \
	    $(RUN_ENV) "$$t" || true; \
	    failures=$$((failures+1)); \
	  fi; \
	done; \
	echo "C tests: $$((total-failures))/$$total passed"; \
	test "$$failures" -eq 0

test-lisp: $(REPL_BIN)
	@set -e; failures=0; total=0; \
	for src in $(TEST_LISP_SRCS); do \
	  total=$$((total+1)); \
	  expected="$${src%.lisp}.expected"; \
	  name=$$(basename "$$src" .lisp); \
	  if [ ! -f "$$expected" ]; then \
	    printf "  \033[33mSKIP\033[0m  %s (no .expected file)\n" "$$name"; \
	    continue; \
	  fi; \
	  actual=$$($(RUN_ENV) $(REPL_BIN) < "$$src" 2>&1) || status=$$?; \
	  status=$${status:-0}; \
	  unset status; \
	  if [ "$$actual" = "$$(cat $$expected)" ]; then \
	    printf "  \033[32mPASS\033[0m  %s\n" "$$name"; \
	  else \
	    printf "  \033[31mFAIL\033[0m  %s\n" "$$name"; \
	    echo "    expected:"; sed 's/^/      /' "$$expected"; \
	    echo "    actual:";   echo "$$actual" | sed 's/^/      /'; \
	    failures=$$((failures+1)); \
	  fi; \
	done; \
	echo "Lisp tests: $$((total-failures))/$$total passed"; \
	test "$$failures" -eq 0
