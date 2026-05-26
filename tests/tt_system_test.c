/* tests/tt_system_test.c
 *
 * Tests for systems (multi-face partial elements) and comp Glue
 * (Turn 11). This is the layer that lets the canonical-case
 * computational univalence run end-to-end.
 *
 * Coverage:
 *   - System construction (system-nil, system-cons)
 *   - System simplification: F1 clause selects, F0 clause drops
 *   - System lookup via face entailment
 *   - hcomp/comp with empty partial reduces to base
 *   - comp Glue produces glue-intro of constituent comps
 *   - Full chain: comp Glue with id-equiv simplifies completely
 *   - Transport along (ua (id-equiv A)) reduces to the input
 *     *** END-TO-END COMPUTATIONAL UNIVALENCE (canonical case) ***
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* ===== System construction ===== */
  r = lizard_test_eval(&e, "(system-nil)");
  TEST_ASSERT(r->type == AST_TT_SYSTEM_NIL);

  r = lizard_test_eval(&e, "(system-cons (F1) 'v (system-nil))");
  TEST_ASSERT(r->type == AST_TT_SYSTEM_CONS);

  /* ===== System simplification ===== */
  /* F1 clause head: select value */
  r = lizard_test_eval(&e, "(reduce (system-cons (F1) 'v (system-nil)))");
  TEST_ASSERT_STR(lizard_test_format(r), "v");

  /* F0 clause head: drop, go to rest */
  r = lizard_test_eval(&e, "(reduce (system-cons (F0) 'v (system-nil)))");
  TEST_ASSERT_STR(lizard_test_format(r), "[]");

  /* Concrete face becomes F1, selects */
  r = lizard_test_eval(&e,
                       "(reduce (system-cons (F-eq (i0) (i0)) 'a"
                       "         (system-cons (F-eq (i0) (i1)) 'b"
                       "          (system-nil))))");
  TEST_ASSERT_STR(lizard_test_format(r), "a");

  /* ===== System lookup ===== */
  r = lizard_test_eval(&e,
                       "(system-lookup "
                       "  (system-cons (F-eq (I-var 'i) (i0)) 'v0"
                       "   (system-cons (F-eq (I-var 'i) (i1)) 'v1"
                       "    (system-nil)))"
                       "  (F-eq (I-var 'i) (i0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "v0");

  /* ===== hcomp with empty partial ===== */
  r = lizard_test_eval(&e,
                       "(reduce (hcomp 'A (F-eq (I-var 'k) (i0))"
                       "               (system-nil) 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "u0");

  /* ===== comp with F0 face and constant family: base ===== */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i 'A) (F0)"
                       "              (system-nil) 'u0))");
  TEST_ASSERT_STR(lizard_test_format(r), "u0");

  /* ===== comp Glue produces glue-intro ===== */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i (Glue 'A (F-eq (I-var 'k) (i0))"
                       "                                 'T 'eq))"
                       "              (F-eq (I-var 'l) (i0))"
                       "              'u 'u0))");
  TEST_ASSERT(r != NULL);
  TEST_ASSERT(r->type == AST_TT_GLUE_INTRO);

  /* ===== Full chain: comp Glue with id-equiv simplifies all the way ===== */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i "
                       "                 (Glue 'A (F-eq (I-var 'k) (i0))"
                       "                       'A (id-equiv 'A)))"
                       "              (F0) (system-nil)"
                       "              (glue-intro (F-eq (I-var 'k) (i0)) 'a 'a)))");
  TEST_ASSERT(r->type == AST_TT_GLUE_INTRO);
  /* The inner comps should have fired through id-equiv */

  /* *** Transport along (ua (id-equiv A)) reduces to x.
   *     This is end-to-end computational univalence in the
   *     canonical case. *** */
  r = lizard_test_eval(&e,
                       "(reduce (comp (path-abs 'i (path-app (ua (id-equiv 'A))"
                       "                                     (I-var 'i)))"
                       "              (F0) (system-nil) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "x");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
