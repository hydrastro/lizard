(import "cas/rtower.lisp")
(define s1 (list (list (quote exp) (rat-one))))
(define t1 (rt-theta 1))
(define sL (list (list (quote prim) (rat-make (list 1) (list 0 1)))))
(define tL (rt-theta 1))
(define oneoverx (rat-make (list 1) (list 0 1)))
(define s2 (list (list (quote exp) (rat-one)) (list (quote exp) (rt-theta 1))))
(define t2 (rt-theta 2))
; derivation: positive and negative powers
(display (rt-equal? 1 (rt-deriv 1 s1 (rt-inv 1 t1)) (rt-neg 1 (rt-inv 1 t1)))) (display " ")
(display (rt-equal? 1 (rt-deriv 1 sL (rt-inv 1 tL)) (rt-neg 1 (rt-mul 1 (rt-lift1 0 oneoverx) (rt-mul 1 (rt-inv 1 tL) (rt-inv 1 tL)))))) (newline)
(display (rt-equal? 2 (rt-deriv 2 s2 (rt-inv 2 t2)) (rt-neg 2 (rt-mul 2 (rt-lift1 1 (rt-theta 1)) (rt-inv 2 t2))))) (newline)
; multi-residue: depth 1 exp, depth 1 log (nested args), depth 2
(display (rt-integrate-logpart-decides? 1 s1 (list (rat-zero) (rat-scale 2 (rat-one))) (list (rat-neg (rat-one)) (rat-zero) (rat-one)))) (display " ")
(display (rt-integrate-logpart-decides? 1 sL (list (rat-make (list 2) (list 0 1))) (list (rat-neg (rat-one)) (rat-zero) (rat-one)))) (newline)
(display (rt-integrate-logpart-decides? 2 s2 (list (rt-zero 1) (rt-mul 1 (rt-from-int 1 2) (rt-theta 1))) (list (rt-neg 1 (rt-one 1)) (rt-zero 1) (rt-one 1)))) (newline)
; soundness: complex residues NOT elementary
(display (equal? (car (rt-integrate-logpart 1 s1 (list (rat-zero) (rat-scale 2 (rat-one))) (list (rat-one) (rat-zero) (rat-one)))) (quote elementary))) (newline)
