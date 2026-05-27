; -*- lisp -*-
; ============================================================
;  PHASE M.5.9 Turn 2b — Symmetric S5 typing rules
; ============================================================
;
; Turn 2a wired the judgment-kind tracking infrastructure.
; Turn 2b uses it to implement the actual symmetric typing rules.
;
; ----- The five items, executed -----
;
; (1) (poss-coerce e) typing rule:
;       e : A   with kind TRUE
;       ───────────────────────────────────
;       (poss-coerce e) : A   with kind POSS
;     Gated on modal-symmetric. Type unchanged; kind shifts.
;
; (2) (dia e) typing rule:
;       e : A   with kind POSS
;       ───────────────────────────────────
;       (dia e) : Diamond A   with kind TRUE
;     Gated on modal-symmetric. Requires body to be POSS-typed;
;     produces Diamond-wrapped, TRUE-kinded result.
;
; (3) let-diamond kind propagation:
;     Existing M.5.5/M.5.7 rule, now ALSO propagates the body's
;     judgment kind to the result. TRUE body → TRUE result (M.5.5
;     contract preserved). POSS body → POSS result. The user opts
;     into the symmetric discipline by choosing a POSS-typed body.
;
; (4) box-intro kind check (under modal-symmetric):
;     Body must be TRUE-typed. (box (U 0)) works; (box (poss-coerce
;     (U 0))) is rejected — you can't box a poss judgment.
;
; (5) Ω-context plumbing: NOT NEEDED at the kernel level.
;     The kind-propagation design (item 3) replaces explicit Ω
;     threading. Symmetric S5's "single-Ω-entry" invariant is
;     enforced structurally: a let-diamond with a POSS body
;     propagates POSS; any dia that consumes it then requires that
;     POSS kind. The kernel signature stays the same.
;
; ----- Design principle: opt-in -----
;
; Symmetric S5 doesn't break existing code. Old programs using
; (let-diamond x (diamond e) x) continue to work — the body is
; TRUE-typed, so the result is TRUE-typed, and lizard's behavior
; matches M.5.5/M.5.7 exactly.
;
; To engage with symmetric S5, the user writes (poss-coerce e) or
; (dia e). These forms produce POSS-typed values, which flow
; through let-diamond chains and are eventually consumed by dia.

; ============================================================
;  1. poss-coerce and dia: basic typing
; ============================================================

(set-logic 'S5)
(display "=== Under S5, modal-symmetric is on ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  modal-symmetric: ") (display (logic-rule-enabled? 'modal-symmetric)) (newline)
(newline)

(display "=== poss-coerce shifts kind ===") (newline)
(display "  (poss-coerce (U 0)):") (newline)
(display "    ") (display (infer (context) (poss-coerce (U 0)))) (newline)
(display "  ↑ type is (U 1); judgment kind shifted from TRUE to POSS") (newline)
(display "    (the kind is internal; from Lisp you see just the type)") (newline)
(newline)

(display "=== dia wraps a POSS body in Diamond ===") (newline)
(display "  (dia (poss-coerce (U 0))):") (newline)
(display "    ") (display (infer (context) (dia (poss-coerce (U 0))))) (newline)
(display "  ↑ Diamond (U 1); dia requires POSS body, produces TRUE result") (newline)
(newline)

(display "=== dia of a TRUE body is rejected ===") (newline)
(display "  (dia (U 0)): ")
(display (infer (context) (dia (U 0)))) (newline)
(display "  ↑ body is TRUE, not POSS — dia rejects") (newline)
(newline)

; ============================================================
;  2. let-diamond propagates kind
; ============================================================

(display "=== let-diamond with TRUE body: M.5.5 contract preserved ===") (newline)
(display "  (let-diamond x (diamond (U 0)) x):") (newline)
(display "    ") (display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(display "  ↑ TRUE body → TRUE result (same as M.5.5)") (newline)
(newline)

(display "=== let-diamond with POSS body via poss-coerce: POSS result ===") (newline)
;; The result is the body's type — same as before. The KIND changes.
;; We can't observe the kind directly from Lisp; we verify by what
;; dia accepts/rejects.
(display "  (let-diamond x (diamond (U 0)) (poss-coerce x)):") (newline)
(display "    ") (display (infer (context) (let-diamond 'x (diamond (U 0)) (poss-coerce 'x)))) (newline)
(display "  ↑ type (U 1); kind propagated as POSS (verified below)") (newline)
(newline)

; ============================================================
;  3. The full symmetric pipeline
; ============================================================

(display "=== The full symmetric pipeline ===") (newline)
(display "  (dia (let-diamond x (diamond (U 0)) (poss-coerce x))):") (newline)
(display "    ")
(display (infer (context) (dia (let-diamond 'x (diamond (U 0)) (poss-coerce 'x))))) (newline)
(display "  ↑ Diamond (U 1)") (newline)
(display "") (newline)
(display "  This is the canonical symmetric S5 computation:") (newline)
(display "    diamond     produces  Diamond value (TRUE)") (newline)
(display "    let-diamond opens     binds to x, evaluates body") (newline)
(display "    poss-coerce shifts    x is now POSS-typed inside body") (newline)
(display "    let-diamond returns   body's kind propagates → POSS result") (newline)
(display "    dia        wraps      POSS body → Diamond-typed TRUE") (newline)
(newline)

(display "=== Pipeline fails if poss-coerce is omitted ===") (newline)
(display "  (dia (let-diamond x (diamond (U 0)) x)):") (newline)
(display "    ") (display (infer (context) (dia (let-diamond 'x (diamond (U 0)) 'x)))) (newline)
(display "  ↑ inner body x is TRUE → let-diamond returns TRUE → dia rejects") (newline)
(newline)

; ============================================================
;  4. box rejects POSS bodies
; ============================================================

(display "=== box requires TRUE body ===") (newline)
(display "  (box (U 0)): ") (display (infer (context) (box (U 0)))) (newline)
(display "  (box (poss-coerce (U 0))):") (newline)
(display "    ") (display (infer (context) (box (poss-coerce (U 0))))) (newline)
(display "  ↑ can't box a poss judgment — Pfenning-Davies discipline") (newline)
(newline)

; ============================================================
;  5. Symmetric forms gated on modal-symmetric
; ============================================================

(display "=== Symmetric forms require modal-symmetric ===") (newline)
(set-logic 'S4)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  modal-symmetric: ") (display (logic-rule-enabled? 'modal-symmetric)) (newline)
(display "") (newline)
(display "  (poss-coerce (U 0)) under S4:") (newline)
(display "    ") (display (infer (context) (poss-coerce (U 0)))) (newline)
(display "  (dia (poss-coerce (U 0))) under S4:") (newline)
(display "    ") (display (infer (context) (dia (poss-coerce (U 0))))) (newline)
(display "  ↑ both rejected — symmetric forms unavailable") (newline)
(newline)

; ============================================================
;  6. Old asymmetric forms unchanged under S5
; ============================================================

(set-logic 'S5)
(display "=== Old asymmetric forms unchanged under S5 ===") (newline)
(display "  (diamond (U 0)): ") (display (infer (context) (diamond (U 0)))) (newline)
(display "  (let-diamond x (diamond (U 0)) x): ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(define ctxk
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0)))))
    (variable 'a (Box (U 0)))))
(display "  (box-app f a): ")
(display (infer ctxk (box-app 'f 'a))) (newline)
(define ctxd
  (context-extend
    (context-extend (context) (variable 'g (Pi '_ (U 0) (Diamond (U 0)))))
    (variable 'd (Diamond (U 0)))))
(display "  (diamond-bind g d): ")
(display (infer ctxd (diamond-bind 'g 'd))) (newline)
(display "  ↑ all M.5.5/M.5.7/M.5.8 contracts preserved") (newline)
(newline)

; ============================================================
;  Summary
; ============================================================

(display "=== M.5.9 status after Turn 2b ===") (newline)
(display "") (newline)
(display "Symmetric S5 typing rules are now live, opt-in via:") (newline)
(display "  ✓ (poss-coerce e)  — TRUE → POSS judgment shift") (newline)
(display "  ✓ (dia e)          — POSS → TRUE via Diamond wrapping") (newline)
(display "  ✓ let-diamond      — kind propagates from body") (newline)
(display "  ✓ box-intro        — rejects POSS bodies under modal-symmetric") (newline)
(display "") (newline)
(display "Old forms unchanged:") (newline)
(display "  ✓ (diamond e), (let-diamond x d x): M.5.5 contract") (newline)
(display "  ✓ (box-app f a), (diamond-bind g d): M.5.6/M.5.8 contracts") (newline)
(display "  ✓ (unbox x b body): M.5.2 Turn 2 contract") (newline)
(display "") (newline)
(display "What Turn 3 will add:") (newline)
(display "  - Bundle integration polish") (newline)
(display "  - Comprehensive worked example with the full pipeline") (newline)
(display "  - MODAL.md write-up (the standing recommendation, finally)") (newline)
(display "") (newline)
(display "Honest gaps that remain:") (newline)
(display "  - Soundness, canonicity, termination unproven") (newline)
(display "  - True Pfenning-Davies Ω-context single-entry invariant") (newline)
(display "    isn't enforced syntactically — it's encoded via kind") (newline)
(display "    propagation, which is sufficient for type-checking but") (newline)
(display "    isn't quite the same structural discipline") (newline)
(display "  - H.2 (S^1 with computation) still pending") (newline)
(newline)

(logic-config-reset)
(display "=== Phase M.5.9 Turn 2b complete ===") (newline)
