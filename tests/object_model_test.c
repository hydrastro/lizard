#include "object_model.h"
#include "test_harness.h"

#include <string.h>

int main(void) {
  lizard_object_model_info_t info;
  int managed;
  int requires_mark;

  TEST_ASSERT_STR(lizard_object_owner_name(LIZARD_OBJECT_OWNER_HEAP), "heap");
  TEST_ASSERT_STR(lizard_object_owner_name(LIZARD_OBJECT_OWNER_C_MALLOC),
                  "c-malloc");
  TEST_ASSERT_STR(lizard_object_trace_policy_name(LIZARD_OBJECT_TRACE_AST),
                  "ast");
  TEST_ASSERT_STR(lizard_object_trace_policy_name(LIZARD_OBJECT_TRACE_NONE),
                  "none");

  managed = lizard_object_owner_is_runtime_managed(LIZARD_OBJECT_OWNER_HEAP);
  TEST_ASSERT_EQ(managed, 1);
  managed = lizard_object_owner_is_runtime_managed(LIZARD_OBJECT_OWNER_C_MALLOC);
  TEST_ASSERT_EQ(managed, 0);

  requires_mark = lizard_object_trace_policy_requires_mark(
      LIZARD_OBJECT_TRACE_AST);
  TEST_ASSERT_EQ(requires_mark, 1);
  requires_mark = lizard_object_trace_policy_requires_mark(
      LIZARD_OBJECT_TRACE_NONE);
  TEST_ASSERT_EQ(requires_mark, 0);

  lizard_object_model_ast_node(&info);
  TEST_ASSERT_STR(info.name, "ast-node");
  TEST_ASSERT_EQ(info.owner, LIZARD_OBJECT_OWNER_HEAP);
  TEST_ASSERT_EQ(info.trace_policy, LIZARD_OBJECT_TRACE_AST);
  TEST_ASSERT_EQ(info.stable_address, 1);
  TEST_ASSERT_EQ(info.movable, 0);
  TEST_ASSERT_EQ(info.explicitly_freed, 0);

  lizard_object_model_report_object(&info);
  TEST_ASSERT_STR(info.name, "report-object");
  TEST_ASSERT_EQ(info.owner, LIZARD_OBJECT_OWNER_C_MALLOC);
  TEST_ASSERT_EQ(info.trace_policy, LIZARD_OBJECT_TRACE_NONE);
  TEST_ASSERT_EQ(info.stable_address, 1);
  TEST_ASSERT_EQ(info.movable, 0);
  TEST_ASSERT_EQ(info.explicitly_freed, 1);

  lizard_object_model_context_object(&info);
  TEST_ASSERT_STR(info.name, "context-object");
  TEST_ASSERT_EQ(info.owner, LIZARD_OBJECT_OWNER_CONTEXT);
  TEST_ASSERT_EQ(info.trace_policy, LIZARD_OBJECT_TRACE_CUSTOM);
  TEST_ASSERT_EQ(info.stable_address, 1);
  TEST_ASSERT_EQ(info.movable, 0);
  TEST_ASSERT_EQ(info.explicitly_freed, 1);

  TEST_RETURN();
}
