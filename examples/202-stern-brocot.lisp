; 202-stern-brocot.lisp -- the Stern-Brocot tree of the positive rationals.
;
; Every positive rational appears exactly once in the Stern-Brocot tree, reached by a
; unique L/R mediant path from 1/1.  This finds the path, reconstructs the rational from
; it, and exposes the run-length encoding -- whose link to continued fractions is striking:
; 355/113 has CF [3;7,16] and Stern-Brocot runs 3,7,15 (the last term less one).  Three
; certificates witness the structure: path round-trip, the Farey-neighbour determinant
; c*b - a*d = 1 at every node, and distinctness of all 2^k rationals at each depth.
; `must` raises on failure.

(import "cas/sternbrocot.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'sb-check-failed)))
(define (all? l) (cond ((null? l) #t) ((car l) (all? (cdr l))) (else #f)))

(display "The Stern-Brocot tree") (newline) (newline)

(display "1. paths and reconstruction") (newline)
(display "    3/2     -> ") (display (path->string (sb-path (/ 3 2)))) (newline)
(display "    2/5     -> ") (display (path->string (sb-path (/ 2 5)))) (newline)
(display "    355/113 -> runs ") (display (sb-runs (/ 355 113))) (newline)
(must "path of 3/2 is RL"        (equal? (sb-path (/ 3 2)) (list 'R 'L)))
(must "path of 2/5 is LLR"       (equal? (sb-path (/ 2 5)) (list 'L 'L 'R)))
(must "path of 7 is RRRRRR"      (equal? (sb-path (/ 7 1)) (list 'R 'R 'R 'R 'R 'R)))
(must "reconstruct 3/2 from RL"  (= (sb-from-path (list 'R 'L)) (/ 3 2)))
(must "reconstruct 355/113"      (= (sb-from-path (sb-path (/ 355 113))) (/ 355 113)))
(newline)

(display "2. continued-fraction connection in the run-lengths") (newline)
(must "355/113 runs are 3,7,15 (CF [3;7,16])" (equal? (sb-runs (/ 355 113)) (list 3 7 15)))
(must "22/7 runs are 3,6 (CF [3;7])"          (equal? (sb-runs (/ 22 7)) (list 3 6)))
(newline)

(display "3. round-trip over every p/q with p,q in 1..14") (newline)
(define (row p) (map (lambda (q) (sb-roundtrip-ok? (/ p q))) (list 1 2 3 4 5 6 7 8 9 10 11 12 13 14)))
(define (rows p) (if (> p 14) #t (and (all? (row p)) (rows (+ p 1)))))
(must "all reconstruct exactly" (rows 1))
(newline)

(display "4. Farey-neighbour invariant and level distinctness") (newline)
(must "Farey det = 1 along path of 355/113" (sb-farey-ok? (/ 355 113)))
(must "Farey det = 1 along path of 8/13"    (sb-farey-ok? (/ 8 13)))
(must "level 1..7: all 2^k rationals distinct and reduced"
      (all? (map level-distinct-ok? (list 1 2 3 4 5 6 7))))
(newline)

(display "all Stern-Brocot checks passed.") (newline)
