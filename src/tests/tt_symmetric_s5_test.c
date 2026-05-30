/* tests/tt_symmetric_s5_test.c
 *
 * Tests for Phase M.5.8:
 *   (a) Hygiene fix for box-app's substitution into dependent Pi
 *       codomain — fresh names checked free in both arg and inner_b
 *   (b) diamond-bind (Diamond's Kleisli composition / monadic bind),
 *       the structural dual of box-app for Diamond
 *
 * diamond-bind typing (non-dependent):
 *   Δ; Γ ⊢ f : Pi _ A (Diamond B)    Δ; Γ ⊢ d : Diamond A
 *   ─────────────────────────────────────────────────────
 *   Δ; Γ ⊢ (diamond-bind f d) : Diamond B
 *
 * Dependent: substitutes (let-diamond y d y) for x in B.
 *
 * Beta:
 *   (diamond-bind (Lambda x body) (diamond a)) → body[a/x]
 *   (where body : Diamond B)
 */

#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);
  lizard_logic_config_reset();

  /* === Hygiene fix for box-app === */

  /* Use a context where the arg name collides with the fresh-name
   * pattern. The hygiene loop should pick a non-colliding name. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  /* Result has form (Box (unbox <fresh> <colliding-name> <fresh>))
   * where <fresh> must NOT be the same as <colliding-name>. We
   * verify by checking that the inferred type is well-formed (Box?)
   * and that infer doesn't error. */
  r = lizard_test_eval(&e,
    "(Box? (infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x)))) "
    "         (variable '__boxapp_1000 (Box (U 0)))) "
    "       (box-app 'f '__boxapp_1000)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  /* Reset to clear the dim counter side-effects. */
  lizard_logic_config_reset();
  lizard_test_eval(&e, "(set-logic 'S4)");

  /* === diamond-bind construction === */

  r = lizard_test_eval(&e, "(diamond-bind 'f 'd)");
  TEST_ASSERT_STR(lizard_test_format(r), "(diamond-bind f d)");

  /* Predicate. */
  r = lizard_test_eval(&e, "(diamond-bind? (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");
  r = lizard_test_eval(&e, "(diamond-bind? (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "#f");

  /* Accessors. */
  r = lizard_test_eval(&e, "(diamond-bind-fun (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "f");
  r = lizard_test_eval(&e, "(diamond-bind-arg (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "d");

  /* === diamond-bind typing: non-dependent === */

  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0))))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 0))");

  /* === diamond-bind typing: dependent === */

  /* Result should be Diamond-typed; exact form depends on fresh-name
   * counter, so we check structure. */
  r = lizard_test_eval(&e,
    "(Diamond? (infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi 'x (U 0) (Diamond 'x)))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'f 'd)))");
  TEST_ASSERT_STR(lizard_test_format(r), "#t");

  /* === diamond-bind rejects wrong arg type === */

  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0))))) "
    "         (variable 'd (Box (U 0)))) "
    "       (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* === diamond-bind rejects fun whose codomain isn't Diamond === */

  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi '_ (U 0) (U 0)))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");

  /* === diamond-bind reduction === */

  r = lizard_test_eval(&e,
    "(reduce (diamond-bind (Lambda 'x (diamond 'x)) (diamond (U 7))))");
  TEST_ASSERT_STR(lizard_test_format(r), "(diamond (U 7))");

  /* === diamond-bind doesn't reduce when arg isn't a literal diamond === */

  r = lizard_test_eval(&e,
    "(reduce (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "(diamond-bind f d)");

  /* === diamond-bind works under K (Diamond elim is unconditional) === */

  lizard_test_eval(&e, "(set-logic 'K)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0))))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 0))");

  /* === Gate on modalities-enabled === */

  lizard_test_eval(&e, "(logic-rule-disable 'modalities-enabled)");
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0))))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r),
                  "Error: unknown AST node type in eval.");
  lizard_test_eval(&e, "(logic-rule-enable 'modalities-enabled)");

  /* === Diamond/Box duality structural check === */

  /* box-app fun expects Box (Pi). diamond-bind fun expects Pi (Diamond).
   * They're structural duals. */
  lizard_test_eval(&e, "(set-logic 'S4)");
  /* box-app: fun is wrapped in Box. */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0))))) "
    "         (variable 'a (Box (U 0)))) "
    "       (box-app 'f 'a))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Box (U 0))");
  /* diamond-bind: fun's CODOMAIN is wrapped in Diamond (the dual). */
  r = lizard_test_eval(&e,
    "(infer (context-extend "
    "         (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0))))) "
    "         (variable 'd (Diamond (U 0)))) "
    "       (diamond-bind 'f 'd))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Diamond (U 0))");

  lizard_logic_config_reset();
  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
