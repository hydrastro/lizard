; -*- lisp -*-
; ============================================================
;  EXAMPLE 113 — Track K: inductive families (Vec, Fin)
; ============================================================
;
; Dependent types in action: types indexed by values. The length
; index on Vec and the bound on Fin make out-of-bounds indexing
; impossible.

(import "inductive.lisp")

(display "=== LENGTH-INDEXED VECTORS ===") (newline)
; Build Vec A 3 = [1, 2, 3]
(define v3 (vcons 1 (vcons 2 (vcons 3 (vnil)))))
(display "  v3 = ") (display (vec->list v3))
(display " : Vec Nat ") (display (vec-len v3)) (newline)
(display "  head: ") (display (vhead v3)) (newline)
(display "  tail: ") (display (vec->list (vtail v3)))
(display " : Vec Nat ") (display (vec-len (vtail v3))) (newline)
(newline)

(display "=== APPEND: Vec m + Vec n = Vec (m+n) ===") (newline)
(define a (list->vec '(1 2)))
(define b (list->vec '(3 4 5)))
(define ab (vappend a b))
(display "  Vec ") (display (vec-len a)) (display " ++ Vec ") (display (vec-len b))
(display " = Vec ") (display (vec-len ab)) (newline)
(display "  result: ") (display (vec->list ab)) (newline)
(display "  (the length index adds: 2 + 3 = 5)") (newline)
(newline)

(display "=== MAP PRESERVES LENGTH ===") (newline)
(define doubled (vmap (lambda (x) (* x 2)) v3))
(display "  vmap (*2) v3 = ") (display (vec->list doubled))
(display " : Vec Nat ") (display (vec-len doubled)) (newline)
(newline)

(display "=== ZIP REQUIRES EQUAL LENGTHS ===") (newline)
(define zipped (vzip (list->vec '(1 2 3)) (list->vec '(a b c))))
(display "  vzip [1 2 3] [a b c] = ") (display (vec->list zipped)) (newline)
(display "  vzip with mismatched lengths:") (newline)
(display "    ") (display (vec->list (vzip (list->vec '(1 2)) (list->vec '(a b c)))))
(newline)
(newline)

(display "=== Fin n: bounded indices ===") (newline)
(display "  fzero (Fin 3): ") (display (fzero 3)) (newline)
(display "  fsuc fzero: ") (display (fsuc (fzero 3))) (newline)
(display "  make-fin 5 2: ") (display (make-fin 5 2)) (newline)
(display "  make-fin 3 5 (out of bounds):") (newline)
(display "    ") (display (make-fin 3 5)) (newline)
(newline)

(display "=== THE PAYOFF: total safe indexing ===") (newline)
; vnth : Vec A n -> Fin n -> A is TOTAL — can't go out of bounds.
(define vec5 (list->vec '(ten twenty thirty forty fifty)))
(display "  vec5 = ") (display (vec->list vec5)) (newline)
(display "  vnth vec5 (Fin 0): ") (display (vnth vec5 (make-fin 5 0))) (newline)
(display "  vnth vec5 (Fin 2): ") (display (vnth vec5 (make-fin 5 2))) (newline)
(display "  vnth vec5 (Fin 4): ") (display (vnth vec5 (make-fin 5 4))) (newline)
(display "  (every Fin 5 is a valid index — no bounds check needed)") (newline)
(newline)

(display "=== DEPENDENT ELIMINATOR (induction on Vec) ===") (newline)
(display "  vsum [1 2 3 4 5] = ")
(display (vsum (list->vec '(1 2 3 4 5)))) (newline)
(display "  length via eliminator: ")
(display (vall-len (list->vec '(a b c d)))) (newline)
(newline)

(display "=== Track K milestone: inductive families ✓ ===") (newline)
