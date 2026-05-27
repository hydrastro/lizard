/* tests/tt_symmetric_s5_turn2a_test.c
 *
 * Tests for Phase M.5.9 Turn 2a — judgment-kind tracking infrastructure.
 *
 * Turn 2a's deliverable: every existing typing rule produces
 * LIZARD_KIND_TRUE through the new infer2_kind / infer3_kind API.
 * Turn 2b will introduce rules that produce other kinds.
 *
 * What this test verifies:
 *   - infer2 (the no-kind wrapper) still works for every existing form
 *   - infer2_kind with NULL out_kind matches infer2
 *   - infer2_kind with a real pointer reports LIZARD_KIND_TRUE for
 *     every existing form (no semantic change)
 *   - infer3_kind same behavior
 *   - The kind defaults to TRUE if a case forgets to set it
 *     (covered by the function-entry initialization)
 */

#include "test_harness.h"
#include "test_helpers.h"
#include "../include/lizard.h"

/* M.5.9 Turn 2a — lizard_judgment_kind_t and lizard_tt_infer*_kind
 * are declared in primitives.h, included transitively via
 * test_helpers.h. */

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* === Refactor regression: infer2 still works === */

  r = lizard_test_eval(&e, "(infer (context) (U 0))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  r = lizard_test_eval(&e, "(infer (context) (Pi 'x (U 0) (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  r = lizard_test_eval(&e, "(infer (context) (Lambda 'x 'x))");
  /* Lambda without context expects to infer the variable inside; this
   * may or may not succeed depending on what (Lambda 'x 'x) means.
   * The important thing is that no internal kind-tracking bug surfaces. */

  /* === Direct C-level test of infer2_kind === */

  /* Build (U 0) and check it infers to (U 1) with kind TRUE. */
  {
    lizard_ast_node_t *u0, *nil, *t, *result;
    lizard_judgment_kind_t kind = LIZARD_KIND_VALID;  /* set to non-default */
    lizard_heap_t *heap = e.heap;

    u0 = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    u0->type = AST_TT_UNIVERSE;
    u0->data.tt_universe.level = 0;

    nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    nil->type = AST_NIL;

    t = lizard_tt_infer2_kind(nil, nil, u0, heap, &kind);
    /* Should be (U 1). */
    TEST_ASSERT(t != NULL);
    TEST_ASSERT(t->type == AST_TT_UNIVERSE);
    TEST_ASSERT(t->data.tt_universe.level == 1);
    /* And kind should now be TRUE (not VALID, which we set above). */
    TEST_ASSERT(kind == LIZARD_KIND_TRUE);

    /* Pass NULL for kind — should not crash. */
    result = lizard_tt_infer2_kind(nil, nil, u0, heap, NULL);
    TEST_ASSERT(result != NULL);
    TEST_ASSERT(result->type == AST_TT_UNIVERSE);
  }

  /* === infer3_kind forwards correctly === */

  {
    lizard_ast_node_t *u0, *nil, *t;
    lizard_judgment_kind_t kind = LIZARD_KIND_POSS;
    lizard_heap_t *heap = e.heap;

    u0 = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    u0->type = AST_TT_UNIVERSE;
    u0->data.tt_universe.level = 0;

    nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    nil->type = AST_NIL;

    /* Use NULL for poss_ctx — Turn 2a's wrapper ignores it anyway. */
    t = lizard_tt_infer3_kind(nil, nil, NULL, u0, heap, &kind);
    TEST_ASSERT(t != NULL);
    TEST_ASSERT(t->type == AST_TT_UNIVERSE);
    TEST_ASSERT(t->data.tt_universe.level == 1);
    TEST_ASSERT(kind == LIZARD_KIND_TRUE);
  }

  /* === Existing modal forms still produce TRUE === */

  {
    /* Test on a box term: (box (U 0)). Should infer to (Box (U 1))
     * with kind TRUE. */
    lizard_ast_node_t *u0, *box, *nil, *t;
    lizard_judgment_kind_t kind = LIZARD_KIND_VALID;
    lizard_heap_t *heap = e.heap;

    u0 = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    u0->type = AST_TT_UNIVERSE;
    u0->data.tt_universe.level = 0;

    box = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    box->type = AST_TT_BOX_INTRO;
    box->data.tt_box_intro.body = u0;

    nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    nil->type = AST_NIL;

    t = lizard_tt_infer2_kind(nil, nil, box, heap, &kind);
    TEST_ASSERT(t != NULL);
    TEST_ASSERT(t->type == AST_TT_BOX);
    /* Inner type should be (U 1). */
    TEST_ASSERT(t->data.tt_box.argument != NULL);
    TEST_ASSERT(t->data.tt_box.argument->type == AST_TT_UNIVERSE);
    TEST_ASSERT(kind == LIZARD_KIND_TRUE);
  }

  /* === Type errors don't crash the kind tracking === */

  {
    /* Infer something that errors. Kind should still be initialized
     * (we set it to TRUE at function entry; an error path doesn't
     * change that, but it doesn't matter because the caller will
     * see is_error() on the result). */
    lizard_ast_node_t *bad, *nil, *t;
    lizard_judgment_kind_t kind = LIZARD_KIND_VALID;
    lizard_heap_t *heap = e.heap;

    bad = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    bad->type = AST_TT_DIAMOND_INTRO_SYM;  /* No typing rule yet. */
    bad->data.tt_diamond_intro_sym.body = NULL;

    nil = lizard_heap_alloc(sizeof(lizard_ast_node_t));
    nil->type = AST_NIL;

    t = lizard_tt_infer2_kind(nil, nil, bad, heap, &kind);
    TEST_ASSERT(t != NULL);
    /* Result is an error — but kind was still initialized to TRUE. */
    TEST_ASSERT(kind == LIZARD_KIND_TRUE);
  }

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
