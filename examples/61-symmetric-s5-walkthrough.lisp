; -*- lisp -*-
; ============================================================
;  EXAMPLE 61 — Full M.5 modal-layer walkthrough
;  (M.5.9 Turn 3 deliverable)
; ============================================================
;
; This example walks the entire modal layer of lizard in one file,
; starting from the most permissive logic and progressively tightening
; the discipline. Each section builds on the previous; if you read
; from top to bottom, you'll see why each axiom exists, what it
; controls, and how the M.5.9 symmetric S5 forms compose with the
; older asymmetric ones.
;
; This is the companion to docs/MODAL.md. The doc explains the
; concepts; this file shows the code.

; ============================================================
;  Section 1 — K, the minimal modal logic
; ============================================================

(set-logic 'K)
(display "=== Section 1: K ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  toggles: t-axiom=OFF, 4-axiom=OFF, 5-axiom=OFF, symmetric=OFF") (newline)
(newline)

(display "K is the WEAKEST modal logic. It has:") (newline)
(display "  - necessitation (box-intro)") (newline)
(display "  - the K-axiom: Box (A -> B) -> Box A -> Box B") (newline)
(display "") (newline)
(display "K does NOT have:") (newline)
(display "  - T-axiom (Box A -> A): cannot extract a value") (newline)
(display "  - 4-axiom (Box A -> Box (Box A)): nested boxes don't collapse") (newline)
(display "  - 5-axiom (Diamond A -> Box (Diamond A))") (newline)
(newline)

;; The K-axiom via box-app: this works in K (it's THE rule of K).
(define ctxK
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0)))))
    (variable 'a (Box (U 0)))))
(display "  K-axiom application: f : Box (A->B), a : Box A") (newline)
(display "    (box-app f a) : ") (display (infer ctxK (box-app 'f 'a))) (newline)
(newline)

;; Extraction fails in K (no T-axiom):
(display "  Extraction via unbox is REJECTED in K:") (newline)
(display "    (unbox 'x (box (U 0)) 'x): ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "  ↑ K's unbox body must stay Box-typed") (newline)
(newline)

;; But structural manipulation under box works:
(display "  Structural manipulation (body stays Box-typed) WORKS:") (newline)
(display "    (unbox 'x (box (U 0)) (box 'x)): ")
(display (infer (context) (unbox 'x (box (U 0)) (box 'x)))) (newline)
(newline)

; ============================================================
;  Section 2 — T, adding the T-axiom
; ============================================================

(set-logic 'T)
(display "=== Section 2: T (adds T-axiom) ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  toggles: t-axiom=ON, 4-axiom=OFF, 5-axiom=OFF, symmetric=OFF") (newline)
(newline)

(display "T adds:") (newline)
(display "  - T-axiom: Box A -> A. Extraction via unbox now works.") (newline)
(newline)

(display "  Extraction in T:") (newline)
(display "    (unbox 'x (box (U 0)) 'x): ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "  ↑ result is (U 1), the extracted value") (newline)
(newline)

(display "But T does NOT yet have the 4-axiom. Nested boxes don't collapse:") (newline)
(display "  (box (box (U 0))) — nested boxes are a different type.") (newline)
;; Verify: box (box (U 0)) typechecks as (Box (Box (U 1)))? Let's see.
(display "    (infer (box (box (U 0)))): ")
(display (infer (context) (box (box (U 0))))) (newline)
(display "  ↑ Box (Box (U 1)) — nesting is observed, not collapsed") (newline)
(newline)

; ============================================================
;  Section 3 — S4, adding the 4-axiom
; ============================================================

(set-logic 'S4)
(display "=== Section 3: S4 (adds 4-axiom) ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  toggles: t-axiom=ON, 4-axiom=ON, 5-axiom=OFF, symmetric=OFF") (newline)
(newline)

(display "S4 adds:") (newline)
(display "  - 4-axiom: Box A -> Box (Box A).") (newline)
(display "    Valid hypotheses survive ARBITRARY nested boxes.") (newline)
(display "    Encoded structurally: box-intro preserves the valid context.") (newline)
(newline)

;; Showcase comonad structure (Box is a comonad under S4):
(display "S4 makes Box a comonad. The operations:") (newline)
(define ctxBox (context-extend (context) (variable 'b (Box (U 0)))))
(display "  extract   : Box A -> A         ; via (unbox y b in y)") (newline)
(display "    (unbox 'y 'b 'y) : ")
(display (infer ctxBox (unbox 'y 'b 'y))) (newline)
(display "  duplicate : Box A -> Box (Box A) ; via (unbox y b in (box (box y)))") (newline)
(display "    (unbox 'y 'b (box (box 'y))) : ")
(display (infer ctxBox (unbox 'y 'b (box (box 'y))))) (newline)
(newline)

(display "Comonad law (1) — extract ∘ duplicate = id — witnessed:") (newline)
(display "  (reduce (unbox 'z (unbox 'y (box (U 0)) (box (box 'y))) 'z)):") (newline)
(display "    ↦ ") (display (reduce (unbox 'z (unbox 'y (box (U 0)) (box (box 'y))) 'z))) (newline)
(display "  ↑ reduces to (box (U 0)) — the original. Not a proof.") (newline)
(newline)

;; Diamond elim under S4 keeps unboxed value in TRUTH context:
(display "Under S4 (no 5-axiom), diamond's let-diamond places x in TRUTH context:") (newline)
(display "  (let-diamond 'x (diamond (U 0)) 'x): ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(newline)

; ============================================================
;  Section 4 — S5, adding the 5-axiom
; ============================================================

(set-logic 'S5)
(display "=== Section 4: S5 (adds 5-axiom) ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  toggles: t-axiom=ON, 4-axiom=ON, 5-axiom=ON, symmetric=ON") (newline)
(newline)

(display "S5 adds:") (newline)
(display "  - 5-axiom: Diamond A -> Box (Diamond A).") (newline)
(display "    A possibility is necessarily a possibility.") (newline)
(display "  - Symmetric forms (modal-symmetric on by default in S5).") (newline)
(newline)

;; The 5-axiom shows up in let-diamond placing x in VALID context:
(display "5-axiom encoded structurally:") (newline)
(display "  Under S5, let-diamond places the unboxed Diamond content in") (newline)
(display "  the VALID context (not just truth). It survives entering a box.") (newline)
(newline)

;; Diamond as a monad — bind operation:
(display "Diamond is a monad under S4+. The Kleisli composition diamond-bind:") (newline)
(define ctxKleisli
  (context-extend
    (context-extend (context) (variable 'g (Pi '_ (U 0) (Diamond (U 0)))))
    (variable 'd (Diamond (U 0)))))
(display "  g : (U 0) -> Diamond (U 0), d : Diamond (U 0)") (newline)
(display "    (diamond-bind 'g 'd) : ")
(display (infer ctxKleisli (diamond-bind 'g 'd))) (newline)
(newline)

(display "Monad law (1) — join ∘ return = id — witnessed:") (newline)
(display "  (reduce (let-diamond 'y (diamond (diamond (U 0))) 'y)):") (newline)
(display "    ↦ ") (display (reduce (let-diamond 'y (diamond (diamond (U 0))) 'y))) (newline)
(display "  ↑ reduces to (diamond (U 0)) — the original. Not a proof.") (newline)
(newline)

; ============================================================
;  Section 5 — Symmetric S5 (Pfenning-Davies)
; ============================================================

(display "=== Section 5: Symmetric S5 (Pfenning-Davies) ===") (newline)
(display "  modal-symmetric: ") (display (logic-rule-enabled? 'modal-symmetric)) (newline)
(newline)

(display "The symmetric calculus uses three judgment forms:") (newline)
(display "  A true   — true in the current world") (newline)
(display "  A valid  — true under all worlds") (newline)
(display "  A poss   — possibly true (in some world)") (newline)
(display "") (newline)
(display "Two new AST forms engage the symmetric discipline:") (newline)
(display "  (poss-coerce e)  — shift TRUE judgment to POSS") (newline)
(display "  (dia e)          — symmetric Diamond intro; requires POSS body") (newline)
(newline)

;; The basic shift:
(display "Basic shift: TRUE -> POSS via poss-coerce") (newline)
(display "  (poss-coerce (U 0)) : ")
(display (infer (context) (poss-coerce (U 0)))) (newline)
(display "  ↑ type unchanged; judgment kind shifted internally") (newline)
(newline)

;; dia requires POSS body:
(display "dia wraps a POSS body in Diamond, producing TRUE result:") (newline)
(display "  (dia (poss-coerce (U 0))) : ")
(display (infer (context) (dia (poss-coerce (U 0))))) (newline)
(newline)

;; What fails:
(display "dia of a TRUE body is REJECTED:") (newline)
(display "  (dia (U 0)) : ")
(display (infer (context) (dia (U 0)))) (newline)
(display "  ↑ the body is TRUE, not POSS. dia needs POSS.") (newline)
(newline)

;; The full pipeline:
(display "=== Section 6 — The full symmetric pipeline ===") (newline)
(newline)

(display "Composing Diamond, let-diamond, poss-coerce, dia:") (newline)
(display "  (dia (let-diamond x (diamond (U 0)) (poss-coerce x))):") (newline)
(display "    ")
(display (infer (context) (dia (let-diamond 'x (diamond (U 0)) (poss-coerce 'x))))) (newline)
(display "  ↑ Diamond (U 1) — the full chain typechecks") (newline)
(display "") (newline)
(display "Step by step:") (newline)
(display "  (diamond (U 0))             : Diamond (U 1)  kind TRUE") (newline)
(display "  (let-diamond x (diamond ...) — binds x as A under valid ctx (5-axiom)") (newline)
(display "    (poss-coerce x))          : (U 1)          kind POSS") (newline)
(display "  let-diamond returns         : (U 1)          kind POSS (propagated)") (newline)
(display "  (dia ...)                   : (Diamond (U 1)) kind TRUE") (newline)
(newline)

;; The chain breaks if poss-coerce is omitted:
(display "If poss-coerce is omitted, the chain breaks:") (newline)
(display "  (dia (let-diamond x (diamond (U 0)) x)):") (newline)
(display "    ")
(display (infer (context) (dia (let-diamond 'x (diamond (U 0)) 'x)))) (newline)
(display "  ↑ inner body x is TRUE -> let-diamond returns TRUE -> dia rejects") (newline)
(newline)

;; box rejects POSS bodies:
(display "Symmetric box-intro also rejects POSS bodies:") (newline)
(display "  (box (poss-coerce (U 0))):") (newline)
(display "    ") (display (infer (context) (box (poss-coerce (U 0))))) (newline)
(display "  ↑ you cannot box a possibility — Pfenning-Davies discipline") (newline)
(newline)

; ============================================================
;  Section 7 — Backward compatibility
; ============================================================

(display "=== Section 7: Backward compatibility ===") (newline)
(newline)

(display "All pre-M.5.9 modal forms continue to work under S5:") (newline)
(display "  (box (U 0)):                       ")
(display (infer (context) (box (U 0)))) (newline)
(display "  (diamond (U 0)):                   ")
(display (infer (context) (diamond (U 0)))) (newline)
(display "  (unbox 'x (box (U 0)) 'x):         ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "  (let-diamond 'x (diamond (U 0)) 'x):")
(display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(define ctxBackcompat
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0)))))
    (variable 'a (Box (U 0)))))
(display "  (box-app 'f 'a):                   ")
(display (infer ctxBackcompat (box-app 'f 'a))) (newline)
(define ctxBackcompat2
  (context-extend
    (context-extend (context) (variable 'g (Pi '_ (U 0) (Diamond (U 0)))))
    (variable 'd (Diamond (U 0)))))
(display "  (diamond-bind 'g 'd):              ")
(display (infer ctxBackcompat2 (diamond-bind 'g 'd))) (newline)
(newline)

(display "All M.5.5/M.5.6/M.5.7/M.5.8 contracts preserved.") (newline)
(newline)

; ============================================================
;  Section 8 — Direct toggle control
; ============================================================

(display "=== Section 8: Direct toggle control ===") (newline)
(newline)

(display "Toggles can be controlled directly, not just via bundles:") (newline)
(logic-config-reset)
(set-logic 'S4)
(display "  Starting from S4, disable t-axiom-enabled:") (newline)
(logic-rule-disable 't-axiom-enabled)
(display "    (unbox 'x (box (U 0)) 'x): ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "    ↑ S4 now behaves like K for unbox extraction") (newline)
(display "    (current-logic): ") (display (current-logic)) (newline)
(display "    ↑ matches no bundle exactly -> custom") (newline)
(newline)

(display "Enable modal-symmetric without modal-5-axiom (hybrid):") (newline)
(logic-config-reset)
(set-logic 'S4)
(logic-rule-enable 'modal-symmetric)
(display "    (current-logic): ") (display (current-logic)) (newline)
(display "    poss-coerce now works under S4:") (newline)
(display "    (poss-coerce (U 0)): ")
(display (infer (context) (poss-coerce (U 0)))) (newline)
(display "    But the 5-axiom for let-diamond is off,") (newline)
(display "    so let-diamond places x in TRUTH ctx (M.5.5 K/T/S4 path).") (newline)
(newline)

; ============================================================
;  Closing
; ============================================================

(logic-config-reset)
(display "=== End of M.5 walkthrough ===") (newline)
(display "") (newline)
(display "What this example demonstrated:") (newline)
(display "  - K's minimal discipline (no extraction via unbox)") (newline)
(display "  - T's T-axiom (extraction works)") (newline)
(display "  - S4's 4-axiom (nested boxes via Δ preservation)") (newline)
(display "  - S5's 5-axiom (let-diamond extends valid ctx)") (newline)
(display "  - S5's symmetric discipline (poss-coerce, dia, kind propagation)") (newline)
(display "  - Comonad witness for Box, monad witness for Diamond") (newline)
(display "  - Backward compatibility — all asymmetric forms unchanged") (newline)
(display "  - Direct toggle control for hybrid configurations") (newline)
(display "") (newline)
(display "For the full theory, see docs/MODAL.md.") (newline)
(display "For honest limitations, see LIMITATIONS.md.") (newline)
