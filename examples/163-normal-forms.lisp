; 163-normal-forms.lisp — kernel/rank/nullspace over Q and integer normal forms.
;
; Completes the linear-algebra pillar with structural decompositions:
;   * rank and a nullspace (kernel) basis over Q, certified by A v = 0;
;   * Hermite Normal Form H = U A, U unimodular (det +/-1), certified by U A = H;
;   * Smith Normal Form D = U A V, U,V unimodular, D diagonal with d_1|d_2|...,
;     the structure theorem for finitely generated abelian groups, certified by
;     U A V = D plus det U = det V = +/-1 and the divisibility chain.
; `must` raises on failure.

(import "cas/normalform.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'nf-check-failed)))
(define (cat M) (if (null? M) '() (append (car M) (cat (cdr M)))))
(define (gcd-list xs) (if (null? xs) 0 (gg (abs (car xs)) (cdr xs))))
(define (gg a xs) (if (null? xs) a (gg (gcd a (abs (car xs))) (cdr xs))))
(define (prod xs) (if (null? xs) 1 (* (car xs) (prod (cdr xs)))))

(define S (list (list 1 2 3) (list 4 5 6) (list 7 8 9)))
(define M (list (list 1 2) (list 3 4)))
(define A3 (list (list 2 4 4) (list -6 6 12) (list 10 4 16)))

(display "kernel/rank/nullspace and integer normal forms") (newline) (newline)

(display "1. rank and nullspace (kernel) over Q") (newline)
(must "rank [[1..9]] = 2"                  (= (mat-rank S) 2))
(must "nullspace basis = {(1,-2,1)}"       (equal? (mat-nullspace S) (list (list 1 -2 1))))
(must "A v = 0 for every kernel vector"    (nullspace-ok? S))
(must "rank-nullity: rank + nullity = 3"   (= (+ (mat-rank S) (length (mat-nullspace S))) 3))
(must "[[1,2],[3,4]] is nonsingular (empty kernel)" (and (= (mat-rank M) 2) (null? (mat-nullspace M))))
(newline)

(display "2. Hermite Normal Form  H = U A,  U unimodular") (newline)
(must "U A = H and U unimodular  ([[1,2],[3,4]])" (hnf-ok? M))
(must "U A = H and U unimodular  (3x3 integer)"   (hnf-ok? A3))
(must "H([[1,2],[3,4]]) = [[1,0],[0,2]]"          (equal? (car (mat-hnf M)) (list (list 1 0) (list 0 2))))
(newline)

(display "3. Smith Normal Form  D = U A V,  U,V unimodular, d_1|d_2|...") (newline)
(must "full certificate  ([[1,2],[3,4]])" (smith-ok? M))
(must "full certificate  ([[2,0],[0,3]])" (smith-ok? (list (list 2 0) (list 0 3))))
(must "full certificate  ([[6,0],[0,4]])" (smith-ok? (list (list 6 0) (list 0 4))))
(must "full certificate  (3x3 integer)"   (smith-ok? A3))
(must "invariants [[2,0],[0,3]] = (1,6)"  (equal? (smith-invariants (list (list 2 0) (list 0 3))) (list 1 6)))
(must "invariants [[6,0],[0,4]] = (2,12)" (equal? (smith-invariants (list (list 6 0) (list 0 4))) (list 2 12)))
(must "invariants 3x3 = (2,2,156)"        (equal? (smith-invariants A3) (list 2 2 156)))
(newline)

(display "4. invariants are forced (independent cross-checks)") (newline)
(must "d_1 = gcd of all entries  ([[6,0],[0,4]])"   (= (car (smith-invariants (list (list 6 0) (list 0 4)))) (gcd-list (cat (list (list 6 0) (list 0 4))))))
(must "product of invariants = |det|  ([[1,2],[3,4]])" (= (prod (smith-invariants M)) (abs (matrix-det M))))
(must "product of invariants = |det|  (3x3)"           (= (prod (smith-invariants A3)) (abs (matrix-det A3))))
(newline)

(display "all normal-form checks passed.") (newline)
