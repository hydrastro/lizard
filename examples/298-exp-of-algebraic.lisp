; RUNG 5: an ENTANGLED tower -- the EXPONENTIAL OF AN ALGEBRAIC FUNCTION, theta = exp(w) with w a field element
; of K = Q(x)[y]/(y^n - g).  Here the exponential sits genuinely on top of the algebraic layer: its logarithmic
; derivative theta'/theta = w' is itself a FIELD element (for exp(sqrt x), w' = 1/(2 sqrt x) lives in K), not
; merely a rational function as in the earlier mixed cases (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT B * theta dx where B in K and theta = exp(w).  The integral is A theta for a field element A
; iff A' + w' A = B -- the Risch differential equation over K with a FIELD-ELEMENT coefficient w'.  Because w' A
; is a full field product, the y-power sectors COUPLE: this is one coupled linear system in all the sector
; coefficients of A (unlike the decoupled sector RDEs when the coefficient is rational).  The answer A is
; certified by A' + w' A = B in the field.
;
; (The coefficient-matching is done by requiring the residual field element to vanish at sample points, which is
; genuinely linear in the unknowns; the field certificate is the exact arbiter.)
(import "cas/algexp.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Exponential of an algebraic function: INT B exp(w) dx = A exp(w), the coupled RDE A' + w'A = B over K.") (newline) (newline)

(define p (list 0 1))   ; g = x, n = 2, so y = sqrt x and w = sqrt x
(define y (sf-y 2))
(define wp (ae-wprime p 2 y))   ; w' = (sqrt x)' = 1/(2 sqrt x), a FIELD element
(display "theta = exp(sqrt x); its logarithmic derivative w' = (sqrt x)' = ") (display wp) (display "  = 1/(2 sqrt x), in K") (newline) (newline)

(display "the classic INT (1/(2 sqrt x)) exp(sqrt x) dx = exp(sqrt x):") (newline)
(define A1 (sf-one 2))
(define B1 (ae-rhs p 2 wp A1))   ; B = w' (since A=1, A'=0)
(chk "the constructive identity certifies (A = 1, so A' + w'A = w')" (ae-certify p 2 wp A1 B1))
(define s1 (ae-solve p 2 wp B1 0))
(display "  ae-solve recovers A = ") (display s1) (display "  = 1, so the integral is exp(sqrt x)") (newline)
(chk "the solver recovers A = 1" (if (equal? s1 (quote none)) #f (sf-equal? s1 A1)))

(display "INT ((1 + sqrt x)/(2 sqrt x)) exp(sqrt x) dx = sqrt(x) exp(sqrt x):") (newline)
(define B2 (ae-rhs p 2 wp y))
(display "  integrand B = A' + w'A for A = sqrt x is ") (display B2) (newline)
(chk "the constructive identity for A = sqrt x certifies" (ae-certify p 2 wp y B2))
(chk "the solver recovers A = sqrt x (sectors couple through w' A)" (if (equal? (ae-solve p 2 wp B2 0) (quote none)) #f (sf-equal? (ae-solve p 2 wp B2 0) y)))

(display "a higher coefficient A = x + sqrt x (sector 0 a degree-1 polynomial):") (newline)
(define Axy (sf-set (sf-set (sf-zeros 2) 0 (rat-from-poly (list 0 1))) 1 (rat-one)))
(define B3 (ae-rhs p 2 wp Axy))
(chk "the solver recovers A = x + sqrt x (degree-1 search, coupled system)" (if (equal? (ae-solve p 2 wp B3 1) (quote none)) #f (sf-equal? (ae-solve p 2 wp B3 1) Axy)))
(define res (ae-integrate p 2 wp B1 2))
(chk "the top-level integrator returns the elementary answer A = 1" (if (equal? (car res) (quote elementary)) (sf-equal? (car (cdr res)) A1) #f))

(display "the cube-root tower theta = exp(x^(1/3)) on y^3 = x:") (newline)
(define p3 (list 0 1))   ; g = x, n = 3, y = x^(1/3)
(define y3 (sf-y 3))
(define wp3 (ae-wprime p3 3 y3))   ; w' = (x^(1/3))' = (1/(3x)) x^(1/3)
(display "  w' = (x^(1/3))' = ") (display wp3) (newline)
(define B5 (ae-rhs p3 3 wp3 y3))   ; A = x^(1/3)
(chk "INT (...) exp(x^(1/3)) dx = x^(1/3) exp(x^(1/3)) recovered, n = 3" (if (equal? (ae-solve p3 3 wp3 B5 0) (quote none)) #f (sf-equal? (ae-solve p3 3 wp3 B5 0) y3)))

(newline)
(display "Exp of an algebraic function: the exponential's logarithmic derivative lives in K, so the Risch") (newline)
(display "equation A' + w'A = B couples the y-power sectors -- a genuinely entangled tower, solved and certified.") (newline)
