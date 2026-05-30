/* tests/tt_symmetric_s5_turn2b_test.c
 *
 * Tests for Phase M.5.9 Turn 2b — symmetric S5 typing rules.
 *
 * What Turn 2b ships:
 *   1. (poss-coerce e) typing rule — shift TRUE to POSS
 *   2. (dia e) typing rule — requires POSS body, produces TRUE Diamond
 *   3. let-diamond kind propagation — TRUE body → TRUE result; POSS body → POSS result
 *   4. box-intro kind check — rejects POSS body when modal-symmetric on
 *   5. Ω context: defer to Turn 3 (kind propagation through let-diamond
 *      replaces explicit Ω plumbing for this turn)
 *
 * Design principle: symmetric S5 is OPT-IN via the new AST forms.
 * Old code using (let-diamond x d x), (box e), etc. continues to
 * work exactly as it did in M.5.5/M.5.7. The kind check on box-intro
 * fires only when the user explicitly engages with poss-coerce or dia.
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* === poss-coerce typing rule === */

  lizard_test_eval(&e, "(set-logic 'S5)");

  /* Coerce a TRUE judgment to POSS — type unchanged, kind shifted. */
  r = lizard_test_eval(&e, "(infer (context) (poss-coerce (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* poss-coerce of a Box-typed term: works (Box is TRUE). */
  r = lizard_test_eval(&e, "(infer (context) (poss-coerce (box (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* poss-coerce rejected when modal-symmetric is off. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(infer (context) (poss-coerce (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(set-logic 'S5)");

  /* === dia typing rule === */

  /* dia requires POSS body. With poss-coerce, the body becomes POSS. */
  r = lizard_test_eval(&e, "(infer (context) (dia (poss-coerce (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 1))");

  /* dia of a TRUE body is rejected. */
  r = lizard_test_eval(&e, "(infer (context) (dia (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* dia of a diamond — diamond is TRUE-typed, so dia rejects. */
  r = lizard_test_eval(&e, "(infer (context) (dia (diamond (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* dia rejected when modal-symmetric is off. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(infer (context) (dia (poss-coerce (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(set-logic 'S5)");

  /* === let-diamond kind propagation === */

  /* TRUE body → TRUE result (M.5.5 contract preserved). */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* POSS body → POSS result. The result kind isn't directly visible
   * from Lisp (only the type is returned). But we can verify by
   * checking that (dia (let-diamond ...)) accepts a let-diamond
   * with a POSS body but rejects one with a TRUE body. */

  /* dia + let-diamond + poss-coerce: composes correctly. */
  r = lizard_test_eval(&e,
    "(infer (context) (dia (let-diamond 'x (diamond (U 0)) (poss-coerce 'x))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 1))");

  /* dia + let-diamond + plain x (TRUE-typed body): rejected. */
  r = lizard_test_eval(&e,
    "(infer (context) (dia (let-diamond 'x (diamond (U 0)) 'x)))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* === box-intro kind check === */

  /* (box (U 0)) — TRUE body — works. */
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");

  /* (box (poss-coerce (U 0))) — POSS body — rejected under S5. */
  r = lizard_test_eval(&e, "(infer (context) (box (poss-coerce (U 0))))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* Under S4, modal-symmetric off, the box kind check doesn't fire
   * (and poss-coerce itself errors first). */
  lizard_test_eval(&e, "(set-logic 'S4)");
  r = lizard_test_eval(&e, "(infer (context) (box (U 0)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 1))");
  lizard_test_eval(&e, "(set-logic 'S5)");

  /* === Backward compat: M.5.5 and M.5.7 contracts preserved === */

  /* let-diamond extracting under S5 still works. */
  r = lizard_test_eval(&e,
    "(infer (context) (let-diamond 'x (diamond (U 0)) 'x))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 1)");

  /* box-app under S5 still works. */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");

  /* diamond-bind under S5 still works. */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'g (Pi '_ (U 0) (Diamond (U 0))))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'g 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 0))");

  /* === Reduction of new forms preserved (Turn 1 behavior) === */

  r = lizard_test_eval(&e, "(reduce (poss-coerce (U 7)))");
  TEST_ASSERT_STR(lizard_test_format(r), "(U 7)");
  r = lizard_test_eval(&e, "(reduce (dia (poss-coerce (U 7))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(dia (U 7))");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
