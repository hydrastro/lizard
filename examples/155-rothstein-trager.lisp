; 155-rothstein-trager.lisp — the logarithmic part of rational-function
; integration by the Rothstein-Trager method, with a fully rational certificate.
;
; For INT p/q dx (q squarefree, deg p < deg q):
;   R(z) = resultant_x(p - z q', q);  for each residue c (root of R),
;   the term is  c * log(gcd(p - c q', q)).
; Rational residues give logs over Q; algebraic residues are handled in Q(c)
; using polynomials with algebraic-number coefficients, the answer being the sum
; over the conjugate residues (a RootSum).  This integrates 1/(x^2-2), 1/(x^2+1),
; 1/(x^3-2), ... -- cases the partial-fraction integrator defers.
;
; Every result is CERTIFIED over Q by the identity  p = sum_c c*g_c'*(q/g_c),
; the conjugate sum computed via Trace_{Q(c)/Q} (Newton's identities).  `must`
; raises on failure, so a wrong antiderivative cannot pass.

(import "cas/rt.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rt-check-failed)))
(define (certifies? p q) (rt-certificate p q (rt-log-part p q)))

(display "Rothstein-Trager logarithmic part (rational trace certificate)") (newline) (newline)

(display "1. rational residues  (logs over Q)") (newline)
(must "INT 1/(x^2-1)          certified" (certifies? (list 1) (list -1 0 1)))
(must "INT 1/((x-1)(x-2))     certified" (certifies? (list 1) (poly-mul (list -1 1) (list -2 1))))
(must "INT (2x)/(x^2-1)       certified" (certifies? (list 0 2) (list -1 0 1)))
(must "INT (3x-1)/((x-2)(x+3)) certified" (certifies? (list -1 3) (poly-mul (list -2 1) (list 3 1))))
(newline)

(display "2. real algebraic residues  (cases the PF integrator defers)") (newline)
(must "INT 1/(x^2-2)          certified" (certifies? (list 1) (list -2 0 1)))
(must "INT 1/(x^2-3)          certified" (certifies? (list 1) (list -3 0 1)))
(must "INT (3x+2)/(x^2-2)     certified" (certifies? (list 2 3) (list -2 0 1)))
(newline)

(display "3. complex residues  (equivalent to arctan)") (newline)
(must "INT 1/(x^2+1)          certified" (certifies? (list 1) (list 1 0 1)))
(must "INT 1/(x^2+x+1)        certified" (certifies? (list 1) (list 1 1 1)))
(newline)

(display "4. higher-degree squarefree denominators") (newline)
(must "INT 1/(x^3-2)          certified" (certifies? (list 1) (list -2 0 0 1)))
(must "INT 1/((x^2-2)(x-1))   certified" (certifies? (list 1) (poly-mul (list -2 0 1) (list -1 1))))
(must "INT 1/(x^4-1 sqfree)   certified" (certifies? (list 1) (list -1 0 0 0 1)))
(newline)

(display "5. structure of the answer") (newline)
(define t1 (rt-log-part (list 1) (list -1 0 1)))            ; 1/(x^2-1): two rational logs
(must "1/(x^2-1) has 2 log terms"        (= (length t1) 2))
(must "both terms are rational logs"     (and (equal? (car (car t1)) 'rlog) (equal? (car (car (cdr t1))) 'rlog)))
(define t2 (rt-log-part (list 1) (list -2 0 1)))            ; 1/(x^2-2): one RootSum
(must "1/(x^2-2) is a single RootSum"    (and (= (length t2) 1) (equal? (car (car t2)) 'alog)))
(newline)

(display "all Rothstein-Trager checks passed.") (newline)
