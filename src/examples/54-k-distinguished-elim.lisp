; -*- lisp -*-
; ============================================================
;  PHASE M.5.6 — K's distinguished elim
;  T-axiom toggle + K-axiom application (box-app)
; ============================================================
;
; M.5 recap:
;   M.5.1 — Box, Diamond as type constructors
;   M.5.2 — Box intro/elim with beta + strict S4 dual context
;   M.5.3 — Named bundles K/T/S4/S5/modal-STLC
;   M.5.4 — modal-4-axiom toggle distinguishes T from S4
;   M.5.5 — Diamond intro/elim + 5-axiom distinguishes S4 from S5
;   M.5.6 (THIS FILE) — K's distinguished elim: K differs from T
;
; ----- What K is and isn't -----
;
; K is the minimal modal logic. It has:
;   - Necessitation: from a theorem A, derive □A (this is box-intro)
;   - K-axiom:       □(A→B) → □A → □B   (necessity distributes)
;
; K does NOT have:
;   - T-axiom: □A → A (you can't extract a value from a Box)
;   - 4-axiom: □A → □□A (lizard's M.5.4 captures this)
;   - 5-axiom: ◇A → □◇A (lizard's M.5.5 Turn 2 captures this)
;
; Until M.5.6, lizard's `unbox` rule encoded the T-axiom directly:
; given b : Box T, you could write (unbox 'x b 'x) to get a value
; of type T. That extraction is exactly the T-axiom.
;
; M.5.6 splits this off:
;   - `t-axiom-enabled` toggle (default ON)
;   - K bundle sets it OFF; T/S4/S5 set it ON
;
; When OFF, unbox's BODY must be of Box type — no extraction. The
; result is still Box-typed. This forces unbox to be a structural
; tool only.
;
; Plus a new AST node `box-app`:
;   (box-app f a) : Box B  given  f : Box (A→B), a : Box A
;
; This is the K-axiom realized as a term. Available in all modal
; logics (it's a theorem in each), but most useful in K where it's
; the only way to manipulate boxed function values.

; ============================================================
;  1. box-app construction and reduction
; ============================================================

(display "=== box-app: the K-axiom as a term ===") (newline)
(display "  (box-app (box 'f) (box 'a)):") (newline)
(display "    ") (display (box-app (box 'f) (box 'a))) (newline)
(newline)

(display "  Reduction: K-axiom distributes through:") (newline)
(display "  (reduce (box-app (box 'f) (box 'a))):") (newline)
(display "    ") (display (reduce (box-app (box 'f) (box 'a)))) (newline)
(display "    ↑ (box (@ f a)) — boxed application") (newline)
(newline)

; ============================================================
;  2. box-app typing
; ============================================================

(display "=== box-app typing ===") (newline)
(define ctxf
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0)))))
    (variable 'a (Box (U 0)))))

(display "  ctx: f : Box ((U 0) → (U 0)), a : Box (U 0)") (newline)
(display "  (box-app 'f 'a) : ")
(display (infer ctxf (box-app 'f 'a))) (newline)
(display "  ↑ Box (U 0) — non-dependent K-axiom") (newline)
(newline)

(display "  Argument type mismatch rejected:") (newline)
(define ctxg
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0)))))
    (variable 'a (Box (U 1)))))
(display "  (box-app 'f 'a) with a : Box (U 1) instead of Box (U 0): ")
(display (infer ctxg (box-app 'f 'a))) (newline)
(newline)

(display "  Dependent Pi rejected (honest scope):") (newline)
(define ctxh
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi 'x (U 0) 'x))))
    (variable 'a (Box (U 0)))))
(display "  (box-app 'f 'a) with f : Box (Pi x:(U 0). x): ")
(display (infer ctxh (box-app 'f 'a))) (newline)
(display "  ↑ dependent Pi in the boxed function is not yet supported") (newline)
(newline)

; ============================================================
;  3. K disables extraction via unbox
; ============================================================

(display "=== Under K (t-axiom-enabled OFF) ===") (newline)
(set-logic 'K)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  t-axiom-enabled: ") (display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(newline)

(display "  Extraction REJECTED:") (newline)
(display "    (unbox 'x (box (U 0)) 'x) : ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "    ↑ body 'x has type (U 0), not Box-typed; K rejects") (newline)
(newline)

(display "  But structural manipulation OK:") (newline)
(display "    (unbox 'x (box (U 0)) (box 'x)) : ")
(display (infer (context) (unbox 'x (box (U 0)) (box 'x)))) (newline)
(display "    ↑ body (box 'x) has type Box (U 0); K accepts") (newline)
(newline)

(display "  box-app works in K (it's the K-axiom!):") (newline)
(display "    (box-app 'f 'a) where f, a : Box _ : ")
(display (infer ctxf (box-app 'f 'a))) (newline)
(newline)

; ============================================================
;  4. T/S4/S5 keep the T-axiom (extraction works)
; ============================================================

(display "=== Under T/S4/S5 (t-axiom-enabled ON) ===") (newline)
(set-logic 'T)
(display "  T: t-axiom-enabled = ")
(display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(display "    (unbox 'x (box (U 0)) 'x) : ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "    ↑ extraction works") (newline)
(newline)

(set-logic 'S4)
(display "  S4: t-axiom-enabled = ")
(display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(display "    Same extraction works") (newline)

(set-logic 'S5)
(display "  S5: t-axiom-enabled = ")
(display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(newline)

; ============================================================
;  5. Direct toggle control
; ============================================================

(display "=== Direct toggle ===") (newline)
(set-logic 'S4)
(display "  S4 default: extraction works.") (newline)

(logic-rule-disable 't-axiom-enabled)
(display "  After disabling t-axiom-enabled (K-like discipline in S4):") (newline)
(display "    (unbox 'x (box (U 0)) 'x): ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "    ↑ S4 with t-axiom off behaves like K for unbox") (newline)
(logic-rule-enable 't-axiom-enabled)
(newline)

; ============================================================
;  6. Bundle distinguishability
; ============================================================

(display "=== All four modal logics now operationally distinct ===") (newline)
(set-logic 'K)
(display "  (set-logic 'K), current-logic: ") (display (current-logic)) (newline)
(set-logic 'T)
(display "  (set-logic 'T), current-logic: ") (display (current-logic)) (newline)
(set-logic 'S4)
(display "  (set-logic 'S4), current-logic: ") (display (current-logic)) (newline)
(set-logic 'S5)
(display "  (set-logic 'S5), current-logic: ") (display (current-logic))
(display " ← still aliases to CoC, see note below") (newline)
(newline)

(display "Why S5 → 'CoC' in reverse lookup:") (newline)
(display "  S5 enables ALL modal toggles. CoC has don't-care for them.") (newline)
(display "  Default-allow convention: don't-care matches default-on state.") (newline)
(display "  Both match → CoC wins by table order.") (newline)
(display "  This asymmetry is documented; doesn't affect typing behavior.") (newline)
(newline)

; ============================================================
;  7. Status
; ============================================================

(display "=== M.5 status after Turn M.5.6 ===") (newline)
(display "") (newline)
(display "All four modal logics now OPERATIONALLY DISTINCT:") (newline)
(display "  ✓ K  — t-axiom off, no 4-axiom, no 5-axiom") (newline)
(display "  ✓ T  — t-axiom on,  no 4-axiom, no 5-axiom") (newline)
(display "  ✓ S4 — t-axiom on,  4-axiom on, no 5-axiom") (newline)
(display "  ✓ S5 — t-axiom on,  4-axiom on, 5-axiom on") (newline)
(display "") (newline)
(display "Three orthogonal toggles, four named bundles:") (newline)
(display "  t-axiom-enabled, modal-4-axiom, modal-5-axiom") (newline)
(display "") (newline)
(display "Plus new typing rule:") (newline)
(display "  box-app — K-axiom (□(A→B) → □A → □B) realized as a term") (newline)
(display "") (newline)
(display "What's still missing for soundness:") (newline)
(display "  - Dependent Pi inside box-app (non-trivial in dependent TT)") (newline)
(display "  - Categorical correspondence (Box comonad, Diamond monad)") (newline)
(display "    — sketched in example 55, not proven") (newline)
(display "  - Standard caveats: canonicity, termination, decidability") (newline)
(display "") (newline)

(logic-config-reset)
(display "=== Phase M.5.6 complete ===") (newline)
