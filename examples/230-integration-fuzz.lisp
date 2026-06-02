; 230-integration-fuzz.lisp -- a deterministic randomized validator that hardens the integration
; stack by checking its INVARIANTS and its COMPLETENESS on constructed-solvable instances.
;
; A linear-congruential generator produces reproducible pseudo-random polynomials.  Three checks:
;   * rational integration: for random A/D (any degrees), rat-integrate-full must keep the Hermite
;     rational part exact, and when all residues are rational the whole answer differentiates back
;     to A/D.  Invariants -- they must hold for EVERY input.
;   * the Risch DE: for random polynomial f and rational h, set g = h' + f h; the solver must then
;     FIND a certifying solution.  A "none" here would be a completeness bug -- the failure mode a
;     differentiation certificate cannot catch -- so we feed solvable instances on purpose.
;   * exponential integration: for random p and rational h, set R = h' + p' h; INT R e^p must come
;     back elementary and certify.
; This validator found two real defects (dropped polynomial part on improper fractions; a
; division-by-zero on constant denominators), both now fixed in ratfull.lisp.  Fixed seeds keep
; the run reproducible.  `must` raises on any shortfall.

(import "cas/fuzzcheck.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'fuzz-check-failed)))

(display "Randomized differentiate-and-check validation of the integration stack") (newline) (newline)

(display "1. rational integration invariants (rat-integrate-full, arbitrary A/D)") (newline)
(must "seed 12345: 10/10 hold" (= (fz-ratint 12345 10 0) 10))
(must "seed 777:   10/10 hold" (= (fz-ratint 777 10 0) 10))
(must "seed 2024:  10/10 hold" (= (fz-ratint 2024 10 0) 10))
(newline)

(display "2. Risch DE completeness (random poly f, h; g = h' + f h)") (newline)
(must "seed 99999: 10/10 solved + certified" (= (fz-rde 99999 10 0) 10))
(must "seed 31337: 10/10 solved + certified" (= (fz-rde 31337 10 0) 10))
(newline)

(display "3. exponential-integral completeness (random p, h; R = h' + p' h)") (newline)
(must "seed 54321: 10/10 elementary + certified" (= (fz-exp 54321 10 0) 10))
(must "seed 8675:  10/10 elementary + certified" (= (fz-exp 8675 10 0) 10))
(newline)

(display "all 70 randomized instances passed.") (newline)
