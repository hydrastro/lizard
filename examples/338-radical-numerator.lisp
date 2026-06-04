; INT P(x)/sqrt(p) dx for an ARBITRARY polynomial numerator P and a monic quadratic radicand p, by reduction of
; order in the algebraic function field K = Q(x)[y]/(y^2 - p).  This extends the quadratic-radical integration
; (degree <= 1 numerators) to any polynomial numerator -- a step into general algebraic-function integration
; (docs/TRAGER_ROADMAP.md, the frontier).
;
; The antiderivative has the form A(x) sqrt(p) + c log(x + b1/2 + sqrt(p)); differentiating inside K and clearing
; sqrt(p) gives the polynomial identity A' p + A p'/2 + c = P, an exact linear system over Q solved by matching
; coefficients.  Every result is certified inside K by the differentiation certificate (af-certify).
(import "cas/rischradn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Reduction of order: INT P(x)/sqrt(p) = A(x) sqrt(p) + c log(x + b1/2 + sqrt(p)), certified inside K.") (newline) (newline)

(display "INT x^2/sqrt(x^2+1) dx = (x/2) sqrt(x^2+1) - (1/2) log(x + sqrt(x^2+1)):") (newline)
(define r (radn-integrate (list 0 0 1) 0 1))
(display "  A coefficients (low->high) = ") (display (car (cdr r))) (display ",  log coefficient c = ") (display (car (cdr (cdr r)))) (newline)
(chk "A = (1/2) x and c = -1/2" (if (= (rn-nth (car (cdr r)) 1) (/ 1 2)) (= (car (cdr (cdr r))) (/ -1 2)) #f))
(chk "certified inside K = Q(x)[y]/(y^2 - (x^2+1))" (radn-certify (list 0 0 1) 0 1 r))

(display "the base case is reproduced: INT 1/sqrt(x^2+1) dx = log(x + sqrt(x^2+1))  (A empty, c = 1):") (newline)
(chk "INT 1/sqrt(x^2+1) certified" (radn-certify (list 1) 0 1 (radn-integrate (list 1) 0 1)))

(display "higher degree: INT x^3/sqrt(x^2+1) dx, certified:") (newline)
(chk "INT x^3/sqrt(x^2+1) certified" (radn-certify (list 0 0 0 1) 0 1 (radn-integrate (list 0 0 0 1) 0 1)))

(display "a general numerator: INT (x^2 + x + 1)/sqrt(x^2+1) dx, certified:") (newline)
(chk "INT (x^2+x+1)/sqrt(x^2+1) certified" (radn-certify (list 1 1 1) 0 1 (radn-integrate (list 1 1 1) 0 1)))

(display "a non-trivial radicand: INT x^2/sqrt(x^2 + 2x + 5) dx, certified:") (newline)
(chk "INT x^2/sqrt(x^2+2x+5) certified" (radn-certify (list 0 0 1) 2 5 (radn-integrate (list 0 0 1) 2 5)))

(newline)
(display "Arbitrary polynomial numerators over a quadratic radical now integrate by reduction of order, the answer") (newline)
(display "an algebraic-part-plus-logarithm in K, certified by differentiation.  Higher-genus radicands (cubic and") (newline)
(display "quartic p -- elliptic and hyperelliptic, mostly non-elementary) are the open summit beyond.") (newline)
