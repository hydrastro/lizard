/* tests/tt_test.c
 *
 * Type-theory notation forms. NOTHING IS CHECKED — these tests verify
 * construction, reflection (type-of, predicates), accessors, and
 * printing only. A `(Pi (x Nat) garbage)` is just as valid here as a
 * `(Pi (x Nat) Bool)`. See the comment in primitives.c for the full
 * caveat.
 */
#include "test_harness.h"
#include "test_helpers.h"

int main(void) {
  lizard_test_env_t e;
  lizard_ast_node_t *r;
  lizard_test_env_init(&e);

  /* Universes. */
  r = lizard_test_eval(&e, "(U 0)");
  TEST_ASSERT(
      lizard_test_is_symbol(lizard_test_eval(&e, "(type-of (U 0))"), "U"));
  TEST_ASSERT_STR(lizard_test_format(r), "(U 0)");
  r = lizard_test_eval(&e, "(U-level (U 7))");
  TEST_ASSERT(lizard_test_is_int(r, 7));
  r = lizard_test_eval(&e, "(U? (U 0))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(U? 'Nat)");
  TEST_ASSERT(lizard_test_is_false(r));

  /* Couniverses — including the negative levels from the proposal. */
  r = lizard_test_eval(&e, "(Uco -2)");
  TEST_ASSERT_STR(lizard_test_format(r), "(Uco -2)");
  r = lizard_test_eval(&e, "(Uco-level (Uco -2))");
  TEST_ASSERT(lizard_test_is_int(r, -2));
  r = lizard_test_eval(&e, "(Uco? (Uco -1))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(U? (Uco 0))"); /* a couniverse is not a universe */
  TEST_ASSERT(lizard_test_is_false(r));

  /* Pi — dependent function type. */
  r = lizard_test_eval(&e, "(Pi 'x 'Nat 'Bool)");
  TEST_ASSERT_STR(lizard_test_format(r), "(Pi (x Nat) Bool)");
  r = lizard_test_eval(&e, "(Pi? (Pi 'x 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(Pi-binder (Pi 'x 'A 'B))");
  TEST_ASSERT(lizard_test_is_symbol(r, "x"));
  r = lizard_test_eval(&e, "(Pi-domain (Pi 'x 'Nat 'Bool))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Nat"));
  r = lizard_test_eval(&e, "(Pi-codomain (Pi 'x 'Nat 'Bool))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Bool"));

  /* Sigma — dependent pair type. */
  r = lizard_test_eval(&e, "(Sigma 'n 'Nat (@ 'Vec 'n))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Sigma (n Nat) (@ Vec n))");
  r = lizard_test_eval(&e, "(Sigma? (Sigma 'n 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(Sigma-binder (Sigma 'n 'A 'B))");
  TEST_ASSERT(lizard_test_is_symbol(r, "n"));

  /* @ — explicit application form. Distinct from Lisp function call.
   * Use `app?`, `app-fun`, `app-arg` for queries — the `@?` / `@-fun`
   * names work too, but only when separated by whitespace from the
   * leading `@`, which is a single-char token. */
  r = lizard_test_eval(&e, "(@ 'f 'a)");
  TEST_ASSERT_STR(lizard_test_format(r), "(@ f a)");
  r = lizard_test_eval(&e, "(app? (@ 'f 'a))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(app-fun (@ 'f 'a))");
  TEST_ASSERT(lizard_test_is_symbol(r, "f"));
  r = lizard_test_eval(&e, "(app-arg (@ 'f 'a))");
  TEST_ASSERT(lizard_test_is_symbol(r, "a"));

  /* Sum — coproduct. */
  r = lizard_test_eval(&e, "(Sum 'Nat 'Bool)");
  TEST_ASSERT_STR(lizard_test_format(r), "(Sum Nat Bool)");
  r = lizard_test_eval(&e, "(Sum? (Sum 'A 'B))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(Sum-left (Sum 'Nat 'Bool))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Nat"));

  /* Id — identity type. */
  r = lizard_test_eval(&e, "(Id 'Nat 2 (@ 'succ 1))");
  TEST_ASSERT_STR(lizard_test_format(r), "(Id Nat 2 (@ succ 1))");
  r = lizard_test_eval(&e, "(Id? (Id 'A 'a 'b))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(Id-domain (Id 'Nat 2 3))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Nat"));
  r = lizard_test_eval(&e, "(Id-a (Id 'Nat 2 3))");
  TEST_ASSERT(lizard_test_is_int(r, 2));
  r = lizard_test_eval(&e, "(Id-b (Id 'Nat 2 3))");
  TEST_ASSERT(lizard_test_is_int(r, 3));

  /* refl. */
  r = lizard_test_eval(&e, "(refl 'x)");
  TEST_ASSERT_STR(lizard_test_format(r), "(refl x)");
  r = lizard_test_eval(&e, "(refl? (refl 'foo))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(refl-value (refl 42))");
  TEST_ASSERT(lizard_test_is_int(r, 42));

  /* Inductive. */
  r = lizard_test_eval(&e, "(Inductive 'Nat 'Z 'S)");
  TEST_ASSERT_STR(lizard_test_format(r), "(Inductive Nat Z S)");
  r = lizard_test_eval(&e, "(Inductive? (Inductive 'X))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(Inductive-name (Inductive 'Bool 'true 'false))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Bool"));
  /* Constructor list. */
  r = lizard_test_eval(
      &e, "(Inductive-constructors (Inductive 'Bool 'true 'false))");
  TEST_ASSERT_STR(lizard_test_format(r), "(true false)");

  /* Coinductive. */
  r = lizard_test_eval(
      &e, "(Coinductive 'Stream (list 'head 'A) (list 'tail 'StreamA))");
  TEST_ASSERT(lizard_test_is_symbol(
      lizard_test_eval(&e, "(Coinductive-name (Coinductive 'Stream))"),
      "Stream"));
  r = lizard_test_eval(&e, "(Coinductive? (Coinductive 'X))");
  TEST_ASSERT(lizard_test_is_true(r));

  /* Annotations. */
  r = lizard_test_eval(&e, "(annot 'x 'Nat)");
  TEST_ASSERT_STR(lizard_test_format(r), "(: x Nat)");
  r = lizard_test_eval(&e, "(annot? (annot 'a 'b))");
  TEST_ASSERT(lizard_test_is_true(r));
  r = lizard_test_eval(&e, "(annot-term (annot 'thing 'Type))");
  TEST_ASSERT(lizard_test_is_symbol(r, "thing"));
  r = lizard_test_eval(&e, "(annot-type (annot 'thing 'Type))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Type"));

  /* type-of returns the right tag for each form. */
  r = lizard_test_eval(&e, "(type-of (U 0))");
  TEST_ASSERT(lizard_test_is_symbol(r, "U"));
  r = lizard_test_eval(&e, "(type-of (Uco -2))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Uco"));
  r = lizard_test_eval(&e, "(type-of (Pi 'x 'A 'B))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Pi"));
  r = lizard_test_eval(&e, "(type-of (Sigma 'x 'A 'B))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Sigma"));
  r = lizard_test_eval(&e, "(type-of (Id 'A 'a 'b))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Id"));
  r = lizard_test_eval(&e, "(type-of (Inductive 'X))");
  TEST_ASSERT(lizard_test_is_symbol(r, "Inductive"));

  /* The forms are first-class values: you can store them in lists,
     pass them to procedures, pattern-match on them. */
  r = lizard_test_eval(
      &e, "(define ts (list (U 0) (Pi 'x 'A 'B) (Sum 'X 'Y)))"
          "(define (length xs) (if (null? xs) 0 (+ 1 (length (cdr xs)))))"
          "(length ts)");
  TEST_ASSERT(lizard_test_is_int(r, 3));

  /* A small piece of metaprogramming: rename every Pi to Sigma in a list. */
  r = lizard_test_eval(
      &e, "(define (pi->sigma t)"
          "  (if (Pi? t)"
          "      (Sigma (Pi-binder t) (Pi-domain t) (Pi-codomain t))"
          "      t))"
          "(Sigma? (pi->sigma (Pi 'x 'A 'B)))");
  TEST_ASSERT(lizard_test_is_true(r));

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
