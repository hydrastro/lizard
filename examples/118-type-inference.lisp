; -*- lisp -*-
; ============================================================
;  EXAMPLE 118 — Track K: Hindley-Milner type inference
; ============================================================
;
; Algorithm W: infer the principal type of expressions with NO
; annotations, via unification and let-polymorphism.

(import "match.lisp")
(import "hm-infer.lisp")

(display "=== LITERALS ===") (newline)
(display "  42 : ") (display (type->string (type-of 42))) (newline)
(display "  #t : ") (display (type->string (type-of #t))) (newline)
(newline)

(display "=== IDENTITY (polymorphic) ===") (newline)
; (lam x x) : t -> t  (a fresh variable; fully polymorphic)
(display "  (lam x x) : ")
(display (type->string (type-of '(lam x x)))) (newline)
(newline)

(display "=== CONST (K combinator) ===") (newline)
; (lam x (lam y x)) : a -> b -> a
(display "  (lam x (lam y x)) : ")
(display (type->string (type-of '(lam x (lam y x))))) (newline)
(newline)

(display "=== APPLICATION ===") (newline)
; ((lam x x) 42) : int  — applying identity to an int yields int
(display "  ((lam x x) 42) : ")
(display (type->string (type-of '(app (lam x x) 42)))) (newline)
; (lam f (lam x (app f x))) : (a -> b) -> a -> b
(display "  (lam f (lam x (app f x))) : ")
(display (type->string (type-of '(lam f (lam x (app f x)))))) (newline)
(newline)

(display "=== LET-POLYMORPHISM ===") (newline)
; let id = (lam x x) in (id id)  — id used at TWO types
(display "  let id = λx.x in (id id):") (newline)
(display "    ")
(display (type->string
          (type-of '(let id (lam x x) (app id id))))) (newline)
(display "  (id is generalized, so it can apply to itself)") (newline)
(newline)

(display "=== PAIRS (product types) ===") (newline)
; (pair-of 1 #t) : (int * bool)
(display "  (pair-of 42 #t) : ")
(display (type->string (type-of '(pair-of 42 #t)))) (newline)
; (lam x (pair-of x x)) : a -> (a * a)
(display "  (lam x (pair-of x x)) : ")
(display (type->string (type-of '(lam x (pair-of x x))))) (newline)
(newline)

(display "=== COMPOSITION ===") (newline)
; compose = (lam f (lam g (lam x (app f (app g x)))))
;   : (b -> c) -> (a -> b) -> a -> c
(display "  compose : ")
(display (type->string
          (type-of '(lam f (lam g (lam x (app f (app g x)))))))) (newline)
(newline)

(display "=== TYPE ERROR DETECTION ===") (newline)
; (app 42 1) — applying a non-function. Should report an error.
(display "  (app 42 1): ") (newline)
(display "    ") (display (type->string (type-of '(app 42 1)))) (newline)
(newline)

(display "=== Track K depth: full HM inference ✓ ===") (newline)
