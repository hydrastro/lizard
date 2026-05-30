/* tests/kernel_surface_test.c
 *
 * End-to-end Surface -> Kernel pipeline: a theorem written in concrete
 * surface syntax is tokenized, parsed, converted to a trusted KernelTerm
 * (kernel_sexp.c), and type-checked by the kernel (kt_check), all via the
 * user-facing `kernel-check` primitive.
 *
 * This exercises the dependent eliminators the kernel can now type
 * (bool_rec / nat_rec / list_rec / maybe_rec / sum_rec / J / absurd) through
 * the surface forms newly added to the converter, including a genuine proof
 * by induction.  Every positive case must check (#t); every adversarial case
 * must be rejected (#f) — demonstrating the surface path is sound, not just
 * expressive.
 */
#include "test_harness.h"
#include "test_helpers.h"

#define CHECKS(src)  TEST_ASSERT(lizard_test_is_true(lizard_test_eval(&e, (src))))
#define REJECTS(src) TEST_ASSERT(lizard_test_is_false(lizard_test_eval(&e, (src))))

int main(void) {
  lizard_test_env_t e;
  lizard_test_env_init(&e);

  /* --- bool_rec: dependent Bool elimination (constant motive -> Nat). --- */
  CHECKS("(kernel-check '(boolrec (lam (b Bool) Nat) zero (succ zero) true) "
         "'Nat)");
  /* adversarial: true-case has type Bool, motive demands Nat. */
  REJECTS("(kernel-check '(boolrec (lam (b Bool) Nat) true (succ zero) true) "
          "'Nat)");

  /* --- nat_rec: recursion with the induction hypothesis. --- */
  CHECKS("(kernel-check "
         "'(natrec (lam (n Nat) Nat) zero "
         "         (lam (k Nat) (lam (ih Nat) (succ (var 0)))) "
         "         (succ (succ zero))) "
         "'Nat)");
  /* adversarial: zero-case is a Bool. */
  REJECTS("(kernel-check "
          "'(natrec (lam (n Nat) Nat) true "
          "         (lam (k Nat) (lam (ih Nat) (succ (var 0)))) zero) "
          "'Nat)");

  /* --- list_rec: a length-shaped fold over an annotated list. --- */
  CHECKS("(kernel-check "
         "'(listrec (lam (xs (List Nat)) Nat) zero "
         "          (lam (h Nat) (lam (t (List Nat)) (lam (ih Nat) (succ (var 0))))) "
         "          (cons zero (nil Nat))) "
         "'Nat)");

  /* --- maybe_rec: scrutinee (just zero) : Maybe Nat. --- */
  CHECKS("(kernel-check "
         "'(mayberec (lam (m (Maybe Nat)) Nat) zero (lam (a Nat) (var 0)) "
         "           (just zero)) "
         "'Nat)");

  /* --- sum_rec: scrutinee (inl zero Bool) : Sum Nat Bool. --- */
  CHECKS("(kernel-check "
         "'(sumrec (lam (s (Sum Nat Bool)) Nat) (lam (a Nat) (var 0)) "
         "         (lam (b Bool) zero) (inl zero Bool)) "
         "'Nat)");

  /* --- J: path induction with a constant motive. --- */
  CHECKS("(kernel-check "
         "'(J (lam (x Nat) (lam (y Nat) (lam (p (Id Nat (var 1) (var 0))) Nat))) "
         "    (lam (x Nat) zero) Nat zero zero (refl zero)) "
         "'Nat)");

  /* --- absurd: ex falso, absurd {Nat} : Empty -> Nat. --- */
  CHECKS("(kernel-check '(absurd Nat) '(Pi (e Empty) Nat))");

  /* --- The headline: a genuine proof BY INDUCTION of a *dependent*
   * statement.  Motive C n := (Id Nat n n), i.e. "for all n, n = n".
   *   - base   : (refl zero)               : C zero
   *   - step k : given ih : C k, produce    : C (succ k)  via (refl (succ k))
   * The whole nat_rec term inhabits C (succ (succ zero)), and the kernel
   * verifies it against that dependent type.  This is reflexivity proven by
   * induction — checked end to end by the trusted core. --- */
  CHECKS("(kernel-check "
         "'(natrec (lam (n Nat) (Id Nat (var 0) (var 0))) "
         "         (refl zero) "
         "         (lam (k Nat) (lam (ih (Id Nat (var 0) (var 0))) (refl (succ (var 1))))) "
         "         (succ (succ zero))) "
         "'(Id Nat (succ (succ zero)) (succ (succ zero))))");
  /* adversarial: claim the same proof inhabits a *false* equation. */
  REJECTS("(kernel-check "
          "'(natrec (lam (n Nat) (Id Nat (var 0) (var 0))) "
          "         (refl zero) "
          "         (lam (k Nat) (lam (ih (Id Nat (var 0) (var 0))) (refl (succ (var 1))))) "
          "         (succ (succ zero))) "
          "'(Id Nat (succ (succ zero)) zero))");

  lizard_test_env_destroy(&e);
  TEST_RETURN();
}
