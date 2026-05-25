/* tests/test_harness.h
 *
 * Tiny header-only test harness. Each test file defines main(),
 * uses TEST_ASSERT / TEST_ASSERT_EQ / TEST_ASSERT_STR, and reports
 * pass/fail via the process exit code (0 = pass).
 *
 * Output is one line per failing assertion; passing tests are silent
 * so the Makefile harness can show a single PASS line per binary.
 */
#ifndef LIZARD_TEST_HARNESS_H
#define LIZARD_TEST_HARNESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int lizard_test_failures = 0;

#define TEST_ASSERT(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__,     \
              #cond);                                                          \
      lizard_test_failures++;                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQ(got, want)                                              \
  do {                                                                         \
    long _g = (long)(got);                                                     \
    long _w = (long)(want);                                                    \
    if (_g != _w) {                                                            \
      fprintf(stderr, "%s:%d: expected %ld, got %ld (%s != %s)\n", __FILE__,   \
              __LINE__, _w, _g, #got, #want);                                  \
      lizard_test_failures++;                                                  \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR(got, want)                                             \
  do {                                                                         \
    const char *_g = (got);                                                    \
    const char *_w = (want);                                                   \
    if (!_g || !_w || strcmp(_g, _w) != 0) {                                   \
      fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__,        \
              __LINE__, _w ? _w : "(null)", _g ? _g : "(null)");               \
      lizard_test_failures++;                                                  \
    }                                                                          \
  } while (0)

#define TEST_RETURN() return (lizard_test_failures == 0) ? 0 : 1

#endif /* LIZARD_TEST_HARNESS_H */
