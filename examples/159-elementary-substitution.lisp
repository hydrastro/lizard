; 159-elementary-substitution.lisp — elementary integration over a transcendental
; monomial by reduction to the complete, certified rational-function integrator.
;
;   primitive (log x):   INT (1/x) R(log x) dx = [ INT R(t) dt ]_{t=log x}
;   exponential (e^x):   INT R(e^x) dx         = [ INT R(u)/u du ]_{u=e^x}
;
; The reduced integral is an ordinary rational-function integral in the monomial,
; which integrate.lisp solves and certifies by differentiating back over Q --
; including the polynomial part, MULTIPLE logarithms, and ARCTANGENTS.  So this
; gives INT 1/(x((log x)^2-1)) = (1/2)log((log x -1)/(log x +1)), INT 1/(x((log
; x)^2+1)) = arctan(log x), and INT e^x/(e^(2x)+1) = arctan(e^x).  Correctness is
; that Q-certificate (in the monomial) plus the substitution theorem.  `must`
; raises on failure; declined cases are reported honestly, never faked.

(import "cas/elem.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'elem-check-failed)))
(define (PLok num den) (elem-certified? (integrate-primitive-log num den)))
(define (EXok num den) (elem-certified? (integrate-exp-rational num den)))
(define (PLstr num den) (elem-result->string (integrate-primitive-log num den)))
(define (EXstr num den) (elem-result->string (integrate-exp-rational num den)))

(display "elementary integration over a monomial (substitution to rational integrator)") (newline) (newline)

(display "1. primitive case  INT (1/x) R(log x) dx  (certified)") (newline)
(must "INT 1/(x log x)          = log log x"        (PLok (list 1) (list 0 1)))
(must "INT (log x)^2/x          = (1/3)(log x)^3"   (PLok (list 0 0 1) (list 1)))
(must "INT 1/(x(log x)^3)       rational"           (PLok (list 1) (list 0 0 0 1)))
(must "INT 1/(x((log x)^2-1))   two logs"           (PLok (list 1) (list -1 0 1)))
(must "INT 1/(x((log x)^2+1))   arctan"             (PLok (list 1) (list 1 0 1)))
(must "INT (3log x+2)/(x((log x)^2-log x-2)) two logs" (PLok (list 2 3) (list -2 -1 1)))
(must "  log log x answer text"  (equal? (PLstr (list 1) (list 0 1)) "log(log(x)) + C"))
(must "  arctan(log x) answer text" (equal? (PLstr (list 1) (list 1 0 1)) "arctan(log(x)) + C"))
(newline)

(display "2. exponential case  INT R(e^x) dx  (certified)") (newline)
(must "INT e^x/(e^x+1)          = log(e^x+1)"       (EXok (list 0 1) (list 1 1)))
(must "INT 1/(e^x+1)            = x - log(e^x+1)"   (EXok (list 1) (list 1 1)))
(must "INT 1/(e^(2x)+1)         two logs"           (EXok (list 1) (list 1 0 1)))
(must "INT e^x/(e^(2x)+1)       = arctan(e^x)"      (EXok (list 0 1) (list 1 0 1)))
(must "INT (e^x-1)/(e^x+1)                  "       (EXok (list -1 1) (list 1 1)))
(must "  log(e^x+1) answer text" (equal? (EXstr (list 0 1) (list 1 1)) "log((e^x) + 1) + C"))
(must "  arctan(e^x) answer text" (equal? (EXstr (list 0 1) (list 1 0 1)) "arctan((e^x)) + C"))
(newline)

(display "3. declines honestly when the reduced integral needs algebraic residues") (newline)
(must "INT 1/(x((log x)^2-2))   declined"  (equal? (car (integrate-primitive-log (list 1) (list -2 0 1))) 'cannot))
(must "  ... and not falsely certified"    (not (PLok (list 1) (list -2 0 1))))
(newline)

(display "all elementary-substitution checks passed.") (newline)
