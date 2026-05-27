; -*- lisp -*-
; ============================================================
;  PHASE M.5.9 Turn 1 — Symmetric S5 infrastructure
; ============================================================
;
; M.5 recap (through M.5.8):
;   M.5.1   — Box, Diamond as type constructors
;   M.5.2   — Box intro/elim with beta + strict S4 dual context
;   M.5.3   — Named bundles K/T/S4/S5/modal-STLC
;   M.5.4   — modal-4-axiom toggle (T vs S4)
;   M.5.5   — Diamond intro/elim + 5-axiom toggle (S4 vs S5)
;   M.5.6   — K's distinguished elim: t-axiom toggle + box-app
;   M.5.7   — Reverse-lookup memory + dependent Pi in box-app
;   M.5.8   — Hygiene fix + diamond-bind (Diamond's Kleisli)
;
; M.5.9 implements TRUE symmetric S5 in the Pfenning-Davies sense.
; This is a multi-turn project. Turn 1 lays the infrastructure;
; Turn 2 wires the typing rules; Turn 3 adds tests/example/bundle.
;
; ----- The three-judgment form -----
;
; Symmetric S5 uses three judgment forms:
;
;     A valid   — necessarily true (under all worlds)
;     A true    — true (in the current world)
;     A poss    — possibly true (in some world)
;
; And three contexts:
;
;     Δ (valid) — list of valid hypotheses
;     Γ (truth) — list of truth hypotheses
;     Ω (poss)  — AT MOST ONE poss hypothesis ("current focus")
;
; The single-Ω constraint is the crucial structural detail. Without
; it, modal soundness fails.
;
; ----- What Turn 1 ships -----
;
; New AST nodes (with descents but no typing yet):
;   (dia e)            — symmetric Diamond introduction
;   (poss-coerce e)    — shift from "true" to "poss" judgment
;
; New toggle:
;   modal-symmetric    — default on; S5 has it on, K/T/S4 have it off
;
; New kernel API (as wrappers in Turn 1):
;   lizard_tt_infer3(Δ, Γ, Ω, t, heap)
;   lizard_tt_check3(Δ, Γ, Ω, t, T, heap)
;
; Turn 1's wrappers ignore Ω and forward to infer2/check2.
; Turn 2 will use Ω for the symmetric typing rules.
;
; ----- What Turn 1 does NOT yet ship -----
;
;   - Typing rules for (dia e), (poss-coerce e)
;     → Turn 2: rules that use Ω
;   - Symmetric Box-intro and let-diamond rules
;     → Turn 2 wires these
;   - Tests for the symmetric typing behavior
;     → Turn 3

; ============================================================
;  1. Construction
; ============================================================

(display "=== Symmetric Diamond intro ===") (newline)
(display "  (dia (U 0)):         ") (display (dia (U 0))) (newline)
(display "  (dia? (dia (U 0))):  ") (display (dia? (dia (U 0)))) (newline)
(display "  (dia? (diamond (U 0))): ") (display (dia? (diamond (U 0)))) (newline)
(display "  ↑ dia is structurally distinct from diamond") (newline)
(newline)

(display "=== Poss coercion ===") (newline)
(display "  (poss-coerce (U 0)): ") (display (poss-coerce (U 0))) (newline)
(display "  (poss-coerce-body (poss-coerce 'x)): ")
(display (poss-coerce-body (poss-coerce 'x))) (newline)
(newline)

; ============================================================
;  2. Reduction
; ============================================================

(display "=== Reduction ===") (newline)
(display "  (reduce (dia (U 7))): ")
(display (reduce (dia (U 7)))) (newline)
(display "  ↑ dia preserves shape; its body reduces but the form stays") (newline)
(newline)

(display "  (reduce (poss-coerce (U 7))): ")
(display (reduce (poss-coerce (U 7)))) (newline)
(display "  ↑ poss-coerce is operationally a no-op — unwraps to body") (newline)
(display "    (the judgment shift is for typing, not computation)") (newline)
(newline)

; ============================================================
;  3. Substitution respects the new forms
; ============================================================

(display "=== Substitution ===") (newline)
(display "  (reduce (unbox 'x (box (U 5)) (dia 'x))):") (newline)
(display "    ") (display (reduce (unbox 'x (box (U 5)) (dia 'x)))) (newline)
(display "  ↑ x is replaced inside dia") (newline)
(newline)

; ============================================================
;  4. Toggle
; ============================================================

(display "=== modal-symmetric toggle ===") (newline)
(set-logic 'S4)
(display "  S4: modal-symmetric = ")
(display (logic-rule-enabled? 'modal-symmetric)) (newline)
(set-logic 'S5)
(display "  S5: modal-symmetric = ")
(display (logic-rule-enabled? 'modal-symmetric)) (newline)
(display "  S5 current-logic: ") (display (current-logic)) (newline)
(newline)

(set-logic 'K)
(display "  K: modal-symmetric = ")
(display (logic-rule-enabled? 'modal-symmetric)) (newline)
(set-logic 'T)
(display "  T: modal-symmetric = ")
(display (logic-rule-enabled? 'modal-symmetric)) (newline)
(newline)

; ============================================================
;  5. Typing — Turn 1: explicit not-yet-implemented
; ============================================================

(display "=== Typing: not yet wired (Turn 2) ===") (newline)
(set-logic 'S5)
(display "  (infer (context) (dia (U 0))):") (newline)
(display "    ") (display (infer (context) (dia (U 0)))) (newline)
(display "  (infer (context) (poss-coerce (U 0))):") (newline)
(display "    ") (display (infer (context) (poss-coerce (U 0)))) (newline)
(newline)

; ============================================================
;  6. Existing forms unaffected
; ============================================================

(display "=== Existing asymmetric forms still work ===") (newline)
(display "  (infer (context) (diamond (U 0))): ")
(display (infer (context) (diamond (U 0)))) (newline)
(display "  (infer (context) (let-diamond 'x (diamond (U 0)) 'x)): ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(display "  ↑ old code keeps the M.5.5/M.5.7 behavior; the toggle") (newline)
(display "    gates only the NEW symmetric AST nodes") (newline)
(newline)

; ============================================================
;  7. The plan
; ============================================================

(display "=== M.5.9 multi-turn plan ===") (newline)
(display "") (newline)
(display "Turn 1 (this file) — infrastructure only:") (newline)
(display "  ✓ AST nodes for (dia e) and (poss-coerce e)") (newline)
(display "  ✓ Full engine descents (subst, alpha-equal, normalize, etc.)") (newline)
(display "  ✓ Lisp primitives (constructors, predicates, accessors)") (newline)
(display "  ✓ Toggle modal-symmetric (defaults on per convention)") (newline)
(display "  ✓ Three-context API: lizard_tt_infer3, lizard_tt_check3") (newline)
(display "    (currently wrappers that forward to infer2/check2)") (newline)
(display "  ✓ Bundle wiring: S5 sets modal-symmetric=1; K/T/S4 set 0") (newline)
(display "  ✓ Old asymmetric forms unaffected") (newline)
(display "") (newline)
(display "Turn 2 — the real work:") (newline)
(display "  - infer3 actually uses Ω for Diamond-intro and let-diamond") (newline)
(display "  - Typing rules for (dia e), (poss-coerce e)") (newline)
(display "  - Judgment-kind tracking (Approach A: pair return)") (newline)
(display "  - Symmetric Box-intro and let-diamond rules") (newline)
(display "  - This is the turn where regressions are likeliest") (newline)
(display "") (newline)
(display "Turn 3 — tests, example, bundle:") (newline)
(display "  - Comprehensive test for symmetric typing") (newline)
(display "  - Worked example showing the symmetry") (newline)
(display "  - Bundle delivered") (newline)
(display "") (newline)
(display "Turn 4 (if needed) — polish + MODAL.md:") (newline)
(display "  - Buffer for any regressions from Turn 2") (newline)
(display "  - Documentation write-up") (newline)
(newline)

(logic-config-reset)
(display "=== Phase M.5.9 Turn 1 complete ===") (newline)
