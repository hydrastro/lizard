; -*- lisp -*-
; ============================================================
;  GLUE TYPES & UA — Turns 9 & 10
;  Computational univalence (canonical case)
; ============================================================
;
; This turn brings in the machinery that lets univalence COMPUTE
; rather than be postulated. The key idea: an equivalence between
; two types yields a path between them in the universe.
;
; The new pieces:
;
;   (Equiv A B)        — type of equivalences from A to B
;   (id-equiv A)       — the identity equivalence on A
;   (equiv-fun e)      — forward direction A → B
;   (equiv-inv e)      — backward direction B → A
;   (Glue A φ T e)     — type that's A outside φ and T (≃ A) on φ
;   (glue-intro φ t a) — introduce a Glue element
;   (unglue e g)       — extract the underlying A-element
;   (ua e)             — the headline: turns Equiv into Path in U
;
; Reduction rules:
;   (Glue A F1 T e) → T                  Glue on F1 is T
;   (Glue A F0 T e) → A                  Glue on F0 is just A
;   (unglue e (glue-intro φ t a)) → a    elim of intro
;   ((equiv-fun (id-equiv A)) x) → x     identity computes
;   ((equiv-inv (id-equiv A)) x) → x
;   (ua (id-equiv A)) @ i → A            canonical ua computes
;
; Typing rules:
;   Equiv A B     : (U n)
;   id-equiv A    : (Equiv A A)
;   equiv-fun e   : (Pi _ A B)   when e : Equiv A B
;   equiv-inv e   : (Pi _ B A)
;   Glue A φ T e  : (U n)
;   glue-intro    : checked against (Glue A φ T e)
;   unglue e g    : codomain of e
;   ua e          : (Path U A B)         <-- COMPUTATIONAL UNIVALENCE
;
; ============================================================
;
; What this means concretely:
;
;   - Before: equivalences and paths were SEPARATE worlds.
;     If A ≃ B (equivalent) and you wanted A = B (equal), you
;     had to *postulate* univalence — adding an axiom and breaking
;     canonicity.
;
;   - After: (ua e) is a *definable term* with type (Path U A B).
;     Univalence is a theorem, not a postulate. Functions that
;     transport along (ua e) compute.
;
; The honest gap: the *general* case of (ua e) @ i for an arbitrary
; equivalence requires the comp-Glue rule, which needs multi-face
; partial machinery. The id-equiv case lands fully here. For non-id
; equivalences, the typing works (ua e : Path U A B), but full
; computation through comp would need the multi-face refinement of
; the comp rules.

; ------------------------------------------------------------
; 1. Equivalences
; ------------------------------------------------------------

(display "=== Equivalences ===") (newline)
(define ctx_AB (context (variable 'A (U 0))
                        (variable 'B (U 0))
                        (variable 'eq (Equiv 'A 'B))))

(display "(Equiv A B) : ") (display (infer ctx_AB (Equiv 'A 'B))) (newline)
(display "(id-equiv A) : ") (display (infer ctx_AB (id-equiv 'A))) (newline)
(display "(equiv-fun eq) : ") (display (infer ctx_AB (equiv-fun 'eq))) (newline)
(display "(equiv-inv eq) : ") (display (infer ctx_AB (equiv-inv 'eq))) (newline)
(newline)

; ------------------------------------------------------------
; 2. id-equiv computes as identity
; ------------------------------------------------------------

(display "=== id-equiv computes ===") (newline)
(display "((equiv-fun (id-equiv A)) x) → ")
(display (reduce (@ (equiv-fun (id-equiv 'A)) 'x))) (newline)
(display "((equiv-inv (id-equiv A)) x) → ")
(display (reduce (@ (equiv-inv (id-equiv 'A)) 'x))) (newline)
(newline)

; ------------------------------------------------------------
; 3. Glue boundary rules
; ------------------------------------------------------------

(display "=== Glue boundary ===") (newline)
(display "(Glue A F1 T eq) → ")
(display (reduce (Glue 'A (F1) 'T 'eq))) (newline)
(display "  (F1: Glue IS T)") (newline)

(display "(Glue A F0 T eq) → ")
(display (reduce (Glue 'A (F0) 'T 'eq))) (newline)
(display "  (F0: Glue degenerates to A)") (newline)
(newline)

; ------------------------------------------------------------
; 4. unglue
; ------------------------------------------------------------

(display "=== unglue of glue-intro ===") (newline)
(display "(unglue eq (glue-intro φ t a)) → ")
(display (reduce (unglue 'eq (glue-intro (F-eq (I-var 'i) (i0)) 't 'a))))
(newline)
(display "  (unglue extracts the underlying A-element)") (newline)
(newline)

; ------------------------------------------------------------
; 5. ua — the headline: equivalences to paths in U
; ------------------------------------------------------------

(display "================================================") (newline)
(display "  UA — COMPUTATIONAL UNIVALENCE") (newline)
(display "================================================") (newline)
(newline)
(display "Type of ua:") (newline)
(display "  Given  eq : Equiv A B") (newline)
(display "  Have   (ua eq) : Path U A B") (newline)
(newline)

(display "(ua eq) : ")
(display (infer ctx_AB (ua 'eq))) (newline)
(display "  ↑↑↑ A path in U from A to B, derived from the equivalence.")
(newline)
(display "  This is what makes univalence COMPUTE rather than be axiomatic.")
(newline)
(newline)

; ------------------------------------------------------------
; 6. The canonical case: ua of identity
; ------------------------------------------------------------

(display "=== Canonical ua: identity ===") (newline)
(display "(ua (id-equiv A)) : ")
(display (infer ctx_AB (ua (id-equiv 'A)))) (newline)
(display "  (a path from A to A via the identity)") (newline)
(newline)

(display "(ua (id-equiv A)) @ i0 = ")
(display (reduce (path-app (ua (id-equiv 'A)) (i0)))) (newline)

(display "(ua (id-equiv A)) @ i1 = ")
(display (reduce (path-app (ua (id-equiv 'A)) (i1)))) (newline)

(display "(ua (id-equiv A)) @ (I-var i) = ")
(display (reduce (path-app (ua (id-equiv 'A)) (I-var 'i)))) (newline)
(display "  (constant path on A — fully computed)") (newline)
(newline)

; ------------------------------------------------------------
; 7. Pluggability
; ------------------------------------------------------------

(display "=== Pluggability ===") (newline)
(flag-set! 'reduce-ua-endpoints #f)
(display "with ua-endpoints OFF: (ua (id-equiv A)) @ i = ")
(display (reduce (path-app (ua (id-equiv 'A)) (I-var 'i)))) (newline)
(flag-set! 'reduce-ua-endpoints #t)
(newline)

; ------------------------------------------------------------
; 8. The complete arc
; ------------------------------------------------------------

(display "================================================") (newline)
(display "  THE ARC: Turns 6 → 10") (newline)
(display "================================================") (newline)
(display "  Turn 6:  Interval, Path, path-abs, path-app") (newline)
(display "  Turn 7:  Faces, partials, connection-on-face") (newline)
(display "  Turn 8:  Kan composition (comp/hcomp/fill)") (newline)
(display "  Turn 9:  Glue types, equivalences") (newline)
(display "  Turn 10: ua — equivalences become paths in U") (newline)
(display "           (Identity case: fully computational.)") (newline)
(newline)

; ------------------------------------------------------------
; 9. Honest scope
; ------------------------------------------------------------

(display "=== Honest scope ===") (newline)
(display "  DONE:") (newline)
(display "    All AST, plumbing, printer for Glue/ua/Equiv") (newline)
(display "    Glue F1/F0 boundary rules") (newline)
(display "    unglue of glue-intro elimination") (newline)
(display "    id-equiv computes as identity") (newline)
(display "    (ua (id-equiv A)) @ i computes to A") (newline)
(display "    ua : Equiv A B → Path U A B typing") (newline)
(display "    Glue typing, Equiv typing, glue-intro check") (newline)
(display "  DEFERRED honest gap:") (newline)
(display "    (ua e) @ i for non-identity e — needs comp Glue") (newline)
(display "    which needs multi-face partial machinery.") (newline)
(display "    The TYPE is there (Path U A B); the computation") (newline)
(display "    for arbitrary equivalences would need another turn") (newline)
(display "    of focused work on the comp Glue rule.") (newline)
