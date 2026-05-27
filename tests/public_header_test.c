#include <lizard.h>
#include "test_harness.h"

int main(void) {
  lizard_runtime_options_t options;
  lizard_runtime_options_default(&options);
  TEST_ASSERT(options.initial_heap_size != 0U);
  TEST_ASSERT(options.max_segment_size != 0U);
  return 0;
}
