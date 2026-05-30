; -*- lisp -*-
; ============================================================
;  EXAMPLE 92 — Track Q: HoTT path algebra
; ============================================================
;
; In Homotopy Type Theory, equality proofs are PATHS. Paths form
; a groupoid: reflexivity (identity), symmetry (reversal), and
; transitivity (composition).

(import "hott.lisp")

(display "=== PATHS AS EQUALITY PROOFS ===") (newline)
(newline)

; A reflexivity path: proof that x = x
(define p (path-refl 'x))
(path-describe "refl_x (x = x)" p)

; Symmetry: reverse a path
(define p-rev (path-sym p))
(path-describe "sym(refl_x)" p-rev)

; Transitivity: compose paths
(define q (path-trans p p-rev))
(path-describe "refl · sym(refl)" q)
(newline)

(display "=== GROUPOID STRUCTURE ===") (newline)
(newline)
(display "  Paths support three operations:") (newline)
(display "    refl  : x = x           (identity)") (newline)
(display "    sym   : (a=b) → (b=a)   (inverse)") (newline)
(display "    trans : (a=b)→(b=c)→(a=c) (composition)") (newline)
(newline)

(display "=== INVOLUTION: sym(sym(p)) ===") (newline)
(define involution-test (law-involution (path-refl 'a)))
(display "  sym(sym(refl_a)): ") (display (car involution-test)) (newline)
(display "  refl_a:           ") (display (car (cdr involution-test))) (newline)
(display "  (these are equal under HoTT computation rules)") (newline)
(newline)

(display "=== GROUPOID LAWS ===") (newline)
(newline)
(display "  Left identity (refl · p = p):") (newline)
(define li (law-left-identity 'a (path-refl 'a)))
(display "    LHS: ") (display (car li)) (newline)
(display "    RHS: ") (display (car (cdr li))) (newline)
(newline)

(display "  Inverse law (p · sym(p) = refl):") (newline)
(define inv (law-inverse 'a (path-refl 'a)))
(display "    LHS: ") (display (car inv)) (newline)
(display "    RHS: ") (display (car (cdr inv))) (newline)
(newline)

(display "=== PATH CHAINING ===") (newline)
(define a-path (path-refl 'a))
(define chained (path-concat (list a-path a-path a-path)))
(display "  Concat 3 paths: ") (display chained) (newline)
(newline)

(display "=== TRANSPORT ===") (newline)
(display "  transport carries a value along a path:") (newline)
(define transported (path-transport (path-refl 'A) 'value))
(display "  transport(refl, value) = ") (display transported) (newline)
(display "  (transport along refl is the identity)") (newline)
(newline)

(display "=== KERNEL IDENTITY TYPES ===") (newline)
(display "  The kernel verifies these properly:") (newline)
(display "  (Id Nat 0 0) : ") (display (kernel-infer '(Id Nat 0 0))) (newline)
(display "  (refl 0) : ") (display (kernel-infer '(refl 0))) (newline)
(display "  Does refl prove 0=0? ")
(display (kernel-check '(refl 0) '(Id Nat 0 0))) (newline)
(newline)

(display "╔══════════════════════════════════════════════╗") (newline)
(display "║  Equality is structure, not just a predicate. ║") (newline)
(display "║  Paths compose, reverse, and transport —      ║") (newline)
(display "║  the heart of Homotopy Type Theory.           ║") (newline)
(display "╚══════════════════════════════════════════════╝") (newline)
