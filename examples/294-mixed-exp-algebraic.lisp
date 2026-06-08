; RUNG 5 (the open summit): MIXED TRANSCENDENTAL-OVER-ALGEBRAIC integration.  The first case is
; INT B * exp(h) dx where the coefficient B is an ALGEBRAIC function -- a field element of K = Q(x)[y]/(y^2 - p)
; -- and h is rational.  This couples the transcendental Risch layer (the exponential on top) with the algebraic
; layer below (docs/TRAGER_ROADMAP.md): the exponential sits over an algebraic coefficient field, the genuine
; mixed-tower situation.
;
; The Risch exponential case: INT B exp(h) dx is elementary with the same exponential iff there is a field
; element A in K with A' + h' A = B, and then INT B exp(h) dx = A exp(h).  This is the Risch differential
; equation with coefficients in the algebraic field K.  On y^2 = p (y' = p' y/(2p)) it decouples by sector into
; two scalar equations over Q(x).  Every answer is certified by differentiating A exp(h) in the field and
; checking A' + h' A = B exactly -- the differentiation certificate is the arbiter.
;
; This example works over the canonical field y^2 = x (y = sqrt x) with h = x (so exp(x)).
(import "cas/mixedexp.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Mixed exp-over-algebraic integration: INT B(x, sqrt x) exp(x) dx, solved over the algebraic field.") (newline) (newline)

(define px (rat-from-poly (list 0 1)))   ; the field y^2 = x
(define hp (rat-from-poly (list 1)))     ; h' = 1, i.e. h = x, the exponential exp(x)

(display "the canonical mixed integral INT ((1 + 2x)/(2 sqrt x)) exp(x) dx = sqrt(x) exp(x):") (newline)
(define Ay (af-make (rat-zero) (rat-from-poly (list 1))))   ; A = y = sqrt x
(define B (mx-rhs px hp Ay))
(display "  the integrand B = (sqrt x)' + sqrt x  has field form ") (display B) (display "  (= (1+2x)/(2 sqrt x))") (newline)
(chk "the constructive identity INT B exp(x) = sqrt(x) exp(x) certifies (A' + h'A = B)" (mx-certify px hp Ay B))

(display "solving the Risch differential equation over the field -- recover A from the integrand B:") (newline)
(define Asol (mx-solve-sqrt hp (af-u B) (af-v B) 0 0))
(display "  mx-solve-sqrt recovers A = ") (display Asol) (display "  = sqrt x") (newline)
(chk "the RDE solver recovers A = sqrt x" (if (equal? Asol (quote none)) #f (af-equal? Asol Ay)))
(define res (mx-integrate-sqrt hp B 2))
(display "  INT B exp(x) dx = ") (display res) (display "  (elementary, A exp(x))") (newline)
(chk "the top-level integrator returns the elementary answer A = sqrt x" (if (equal? (car res) (quote elementary)) (af-equal? (car (cdr res)) Ay) #f))

(display "a richer coefficient, A = x + sqrt x (so INT ((x+sqrt x)' + x + sqrt x) exp(x) = (x + sqrt x) exp(x)):") (newline)
(define Axy (af-make (rat-from-poly (list 0 1)) (rat-from-poly (list 1))))   ; x + y
(define B2 (mx-rhs px hp Axy))
(chk "the RDE solver recovers A = x + sqrt x" (if (equal? (mx-solve-sqrt hp (af-u B2) (af-v B2) 1 0) (quote none)) #f (af-equal? (mx-solve-sqrt hp (af-u B2) (af-v B2) 1 0) Axy)))

(display "a higher-degree coefficient A = x^2 + sqrt x, and the honest degree search:") (newline)
(define Ahi (af-make (rat-from-poly (list 0 0 1)) (rat-from-poly (list 1))))   ; x^2 + y
(define B3 (mx-rhs px hp Ahi))
(chk "searching only degree 0 for the polynomial part fails (honest none)" (equal? (mx-solve-sqrt hp (af-u B3) (af-v B3) 0 0) (quote none)))
(chk "searching degree 2 succeeds and recovers A = x^2 + sqrt x" (if (equal? (mx-solve-sqrt hp (af-u B3) (af-v B3) 2 0) (quote none)) #f (af-equal? (mx-solve-sqrt hp (af-u B3) (af-v B3) 2 0) Ahi)))

(newline)
(display "Rung 5 begins: a transcendental exponential integrated over an algebraic coefficient field, by solving") (newline)
(display "the Risch differential equation A' + h'A = B inside K = Q(x)[sqrt x], every answer differentiate-certified.") (newline)
