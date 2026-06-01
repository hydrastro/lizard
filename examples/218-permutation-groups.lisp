; 218-permutation-groups.lisp -- finite permutation groups on {0,...,n-1}.
;
; A permutation is its list of images, so the identity on n points is (0 1 ... n-1) and
; composition is (perm-compose p q) k = p(q k).  A set of generators generates a subgroup of
; S_n, enumerated by breadth-first search over its Cayley graph; the group order is the count.
; That count is then checked a second, independent way -- the orbit-stabilizer theorem
; |G| = |orbit(x)| * |stabilizer(x)| -- and the group axioms (identity, closure, inverses) are
; verified on the enumerated set.  Known orders pin it down: S_3 = 6, S_4 = 24, A_4 = 12,
; D_4 = 8.  `must` raises on failure.

(import "cas/permgroup.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pg-check-failed)))

(display "finite permutation groups") (newline) (newline)

(display "1. permutation arithmetic on {0,1,2}") (newline)
(define a (list 1 0 2))   ; transposition (0 1)
(define b (list 0 2 1))   ; transposition (1 2)
(display "    (0 1) compose (1 2) = ") (display (perm-compose a b)) (display ", the 3-cycle ") (display (perm-cycles (perm-compose a b))) (newline)
(must "(0 1) is an involution" (= (perm-element-order a) 2))
(must "the 3-cycle has order 3" (= (perm-element-order (perm-compose a b)) 3))
(must "p composed with its inverse is the identity" (perm-is-id? (perm-compose a (perm-inverse a))))
(newline)

(display "2. the symmetric group S_3 = <(0 1), (1 2)>") (newline)
(define gens (list a b)) (define n 3)
(display "    ") (display (perm-info gens n)) (newline)
(must "S_3 has order 6 = 3!" (= (group-order gens n) 6))
(must "the enumerated set contains the identity" (closure-has-id? gens n))
(must "it is closed under composition by the generators" (closure-closed? gens n))
(must "it is closed under inverses" (closure-inverse-closed? gens n))
(newline)

(display "3. the orbit-stabilizer theorem, |G| = |orbit| * |stabilizer|") (newline)
(display "    orbit of point 0 under S_3 has size ") (display (pg-len (orbit 0 gens n)))
(display ", stabilizer has order ") (display (pg-len (stabilizer 0 gens n))) (newline)
(must "|orbit| * |stab| = |G| for S_3"  (orbit-stabilizer-ok? 0 gens n))
(must "Lagrange: |stabilizer| divides |G|" (lagrange-ok? 0 gens n))
(newline)

(display "4. larger groups, identified by their known orders") (newline)
(define s4 (list (list 1 0 2 3) (list 1 2 3 0)))     ; S_4
(define a4 (list (list 1 2 0 3) (list 0 2 3 1)))     ; A_4
(define d4 (list (list 1 2 3 0) (list 0 3 2 1)))     ; D_4
(must "S_4 = <(0 1), (0 1 2 3)> has order 24" (= (group-order s4 4) 24))
(must "A_4 = <(0 1 2), (1 2 3)> has order 12" (= (group-order a4 4) 12))
(must "A_4 is a proper subgroup of S_4"       (< (group-order a4 4) (group-order s4 4)))
(must "D_4 (symmetries of the square) has order 8" (= (group-order d4 4) 8))
(must "orbit-stabilizer holds for S_4" (orbit-stabilizer-ok? 0 s4 4))
(newline)

(display "all permutation-group checks passed.") (newline)
