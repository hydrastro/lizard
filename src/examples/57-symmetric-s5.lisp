; -*- lisp -*-
; ============================================================
;  PHASE M.5.8 — Hygiene fix + Diamond's structural dual
; ============================================================
;
; M.5 recap (through M.5.7):
;   M.5.1 — Box, Diamond as type constructors
;   M.5.2 — Box intro/elim with beta + strict S4 dual context
;   M.5.3 — Named bundles K/T/S4/S5/modal-STLC
;   M.5.4 — modal-4-axiom toggle (T vs S4)
;   M.5.5 — Diamond intro/elim + 5-axiom toggle (S4 vs S5)
;   M.5.6 — K's distinguished elim: t-axiom toggle + box-app
;   M.5.7 — Reverse-lookup memory + dependent Pi in box-app
;
; M.5.8 has two pieces:
;
; (1) Hygiene fix: the fresh-name generator in box-app now checks
;     that the candidate name is FREE in both arg and inner_b. If a
;     collision is found, the dim counter is bumped until a non-
;     colliding name is produced. Previously, the freshness relied
;     on users not writing __boxapp_N names.
;
; (2) diamond-bind: the structural dual of box-app for Diamond.
;     Realizes the Kleisli composition / monadic bind for Diamond.
;     Available in all S4+ modal logics.
;
; ----- A note on full symmetric S5 -----
;
; True Pfenning-Davies-style symmetric S5 would split the kernel's
; context discipline to track BOTH "valid" and "possible" judgments
; symmetrically. That is a multi-turn kernel refactor (roughly the
; size of M.5.2 Turn 2's original dual-context split).
;
; M.5.8 takes a more modest path: rather than restructure the
; judgment shape, we add the structural operations (box-app for □,
; diamond-bind for ◇) that demonstrate the duality at the term
; level. The judgment shape remains asymmetric, but the operations
; one can build are now symmetric.

; ============================================================
;  1. Hygiene fix for box-app
; ============================================================

(display "=== Hygiene: arg with colliding name forces retry ===") (newline)
(set-logic 'S4)
;; The literal `__boxapp_1000' should be skipped by the fresh-name
;; generator because the arg expression mentions it.
(define ctx_collide
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x))))
    (variable '__boxapp_1000 (Box (U 0)))))
(display "  Context has __boxapp_1000 as a free variable.") (newline)
(display "  (box-app 'f '__boxapp_1000):") (newline)
(display "    ") (display (infer ctx_collide (box-app 'f '__boxapp_1000))) (newline)
(display "  ↑ fresh binder is __boxapp_1001 (or later), NOT 1000;") (newline)
(display "    the scrutinee __boxapp_1000 is preserved correctly") (newline)
(newline)

(display "=== Normal case still works ===") (newline)
(define ctx_dep
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x))))
    (variable 'a (Box (U 0)))))
(display "  (box-app 'f 'a): ")
(display (infer ctx_dep (box-app 'f 'a))) (newline)
(newline)

; ============================================================
;  2. diamond-bind: Diamond's structural dual of box-app
; ============================================================

(display "=== diamond-bind shape ===") (newline)
(display "") (newline)
(display "  Compare the two structural duals:") (newline)
(display "") (newline)
(display "    box-app:      f : Box (Pi x A B)") (newline)
(display "                  a : Box A") (newline)
(display "                  ──────────────────────") (newline)
(display "                  (box-app f a) : Box B") (newline)
(display "") (newline)
(display "    diamond-bind: f : Pi x A (Diamond B)") (newline)
(display "                  d : Diamond A") (newline)
(display "                  ────────────────────────────") (newline)
(display "                  (diamond-bind f d) : Diamond B") (newline)
(display "") (newline)
(display "  Note the duality:") (newline)
(display "    Box wraps the WHOLE function in box-app.") (newline)
(display "    Diamond wraps the CODOMAIN in diamond-bind.") (newline)
(display "  This matches Box being a co-monad / Diamond being a monad.") (newline)
(newline)

; ============================================================
;  3. diamond-bind typing — non-dependent
; ============================================================

(display "=== Non-dependent typing ===") (newline)
(define ctx_db_nd
  (context-extend
    (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0)))))
    (variable 'd (Diamond (U 0)))))
(display "  f : Pi _ (U 0). Diamond (U 0), d : Diamond (U 0)") (newline)
(display "  (diamond-bind 'f 'd): ")
(display (infer ctx_db_nd (diamond-bind 'f 'd))) (newline)
(newline)

; ============================================================
;  4. diamond-bind typing — dependent
; ============================================================

(display "=== Dependent typing ===") (newline)
(define ctx_db_dep
  (context-extend
    (context-extend (context) (variable 'f (Pi 'x (U 0) (Diamond 'x))))
    (variable 'd (Diamond (U 0)))))
(display "  f : Pi x:(U 0). Diamond x, d : Diamond (U 0)") (newline)
(display "  (diamond-bind 'f 'd):") (newline)
(display "    ") (display (infer ctx_db_dep (diamond-bind 'f 'd))) (newline)
(display "  ↑ substitutes (let-diamond <fresh> d <fresh>) into B = x") (newline)
(newline)

; ============================================================
;  5. Beta reduction
; ============================================================

(display "=== Beta reduction ===") (newline)
(display "  (diamond-bind (Lambda 'x (diamond 'x)) (diamond (U 7))):") (newline)
(display "    ↦ ") (display (reduce (diamond-bind (Lambda 'x (diamond 'x)) (diamond (U 7))))) (newline)
(display "  ↑ K-rule: (@ (Lambda x (diamond x)) (U 7)) → (diamond (U 7))") (newline)
(newline)

; ============================================================
;  6. Cross-modal rejection
; ============================================================

(display "=== diamond-bind rejects Box-typed arg ===") (newline)
(display "  Replace d with Box value:") (newline)
(define ctx_db_bad
  (context-extend
    (context-extend (context) (variable 'f (Pi '_ (U 0) (Diamond (U 0)))))
    (variable 'b (Box (U 0)))))
(display "    ") (display (infer ctx_db_bad (diamond-bind 'f 'b))) (newline)
(newline)

; ============================================================
;  7. Works under K (Diamond elim is unconditional)
; ============================================================

(display "=== diamond-bind under K ===") (newline)
(set-logic 'K)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  t-axiom-enabled (governs Box extract): ")
(display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(newline)

(display "  diamond-bind non-dependent: ")
(display (infer ctx_db_nd (diamond-bind 'f 'd))) (newline)
(display "  ↑ works in K because Diamond elim is unconditional") (newline)
(display "    (Diamond's let-diamond rule needs no T-axiom analogue)") (newline)
(newline)

; ============================================================
;  8. Categorical structure summary
; ============================================================

(display "=== Categorical structure (operations only, not proven laws) ===") (newline)
(display "") (newline)
(display "Box  — comonad (under S4):") (newline)
(display "  extract   : Box A → A") (newline)
(display "  duplicate : Box A → Box (Box A)") (newline)
(display "  fmap      : (A → B) → Box A → Box B   (via box-app + return)") (newline)
(display "  cobind    : (Box A → B) → Box A → Box B  (composes the above)") (newline)
(display "") (newline)
(display "Diamond — monad (under S4+, Kleisli structure via diamond-bind):") (newline)
(display "  return    : A → Diamond A         (just diamond)") (newline)
(display "  join      : Diamond (Diamond A) → Diamond A") (newline)
(display "                                     (via let-diamond x dd x)") (newline)
(display "  bind      : Diamond A → (A → Diamond B) → Diamond B") (newline)
(display "                                     (diamond-bind)") (newline)
(display "  fmap      : (A → B) → Diamond A → Diamond B") (newline)
(display "                                     (via bind + (return ∘ f))") (newline)
(display "") (newline)
(display "Reminder: laws not proven in lizard; the operations are") (newline)
(display "definable at the right types and witnesses fire on specific") (newline)
(display "terms. See example 55 for law witnesses.") (newline)
(newline)

; ============================================================
;  9. What's still missing for full symmetric S5
; ============================================================

(display "=== Honest scope for M.5.8 ===") (newline)
(display "") (newline)
(display "What this turn adds:") (newline)
(display "  ✓ Hygiene fix for box-app's fresh-name generator") (newline)
(display "  ✓ diamond-bind as a first-class form (Kleisli composition)") (newline)
(display "  ✓ Beta rule for diamond-bind") (newline)
(display "  ✓ Dependent codomain via let-diamond substitution") (newline)
(display "  ✓ Demonstrates structural duality with box-app") (newline)
(display "") (newline)
(display "What full Pfenning-Davies symmetric S5 would require:") (newline)
(display "  ✗ A third context dimension (\"poss\" alongside valid/truth)") (newline)
(display "  ✗ Box and Diamond intro rules dually scoped") (newline)
(display "  ✗ Multi-turn kernel refactor (size of M.5.2 Turn 2)") (newline)
(display "  ✗ Different downstream implications for many existing rules") (newline)
(display "") (newline)
(display "M.5.8 takes the pragmatic path: dual operations at the term") (newline)
(display "level, without restructuring the judgment shape. The kernel") (newline)
(display "remains in the Pfenning-Davies asymmetric S4-with-5-axiom") (newline)
(display "discipline; the user-visible operations are symmetric.") (newline)
(newline)

(logic-config-reset)
(display "=== Phase M.5.8 complete ===") (newline)
