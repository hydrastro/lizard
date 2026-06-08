; RUNG 5: MIXED EXPONENTIAL-OVER-ALGEBRAIC integration over the GENERAL superelliptic field K = Q(x)[y]/(y^n - g)
; for arbitrary n.  This lifts the n = 2 case (the sqrt field of mixedexp.lisp) to any degree, using the
; general-n field arithmetic of sefield.lisp (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT B * exp(h) dx where B is a field element of K and h is rational; the integral is A exp(h) for
; a field element A iff A' + h' A = B in K.  Writing A = sum_j a_j y^j, the sefield derivation preserves each
; y^j sector, so A' + h' A = sum_j [a_j' + ((j/n) g'/g + h') a_j] y^j, and matching B = sum_j B_j y^j gives n
; INDEPENDENT scalar Risch differential equations a_j' + w_j a_j = B_j (w_j = (j/n) g'/g + h').  The sectors
; decouple completely; each is solved by undetermined coefficients and the assembled A is differentiate-certified.
(import "cas/mixedexpn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Mixed exp-over-algebraic over y^n = g for general n: INT B exp(h) dx = A exp(h), solved per sector.") (newline) (newline)

(define g (list 1 0 0 1))        ; g = x^3 + 1, the cube-root curve y^3 = x^3 + 1
(define hp (rat-from-poly (list 1)))   ; h' = 1, the exponential exp(x)

(display "the cube-root case INT ((1 + x^2/(x^3+1)) y) exp(x) dx = y exp(x) = (x^3+1)^(1/3) exp(x):") (newline)
(define Ay (sf-y 3))   ; A = y
(define B (mxn-rhs g 3 hp Ay))
(display "  integrand B = y' + y has y-coefficient ") (display (mxn-nth B 1)) (display "  = (x^3+x^2+1)/(x^3+1)") (newline)
(chk "the constructive identity INT B exp(x) = y exp(x) certifies in K" (mxn-certify g 3 hp Ay B))

(display "solving the per-sector RDEs -- recover A from B:") (newline)
(define Asol (mxn-solve g 3 hp B 0))
(display "  mxn-solve recovers A = ") (display Asol) (display "  = y") (newline)
(chk "the solver recovers A = y (sectors decouple: only sector 1 is nonzero)" (if (equal? Asol (quote none)) #f (sf-equal? Asol Ay)))
(define res (mxn-integrate g 3 hp B 2))
(chk "the top-level integrator returns the elementary answer A = y" (if (equal? (car res) (quote elementary)) (sf-equal? (car (cdr res)) Ay) #f))

(display "the y^2 sector, INT (...) exp(x) dx = y^2 exp(x):") (newline)
(define Ay2 (sf-set (sf-zeros 3) 2 (rat-one)))   ; y^2
(define B2 (mxn-rhs g 3 hp Ay2))
(chk "the solver recovers A = y^2" (if (equal? (mxn-solve g 3 hp B2 0) (quote none)) #f (sf-equal? (mxn-solve g 3 hp B2 0) Ay2)))

(display "all three sectors at once, A = x + y + y^2:") (newline)
(define Amix (sf-set (sf-set (sf-set (sf-zeros 3) 0 (rat-from-poly (list 0 1))) 1 (rat-one)) 2 (rat-one)))
(define B3 (mxn-rhs g 3 hp Amix))
(chk "the solver recovers A = x + y + y^2 (independent sector solves)" (if (equal? (mxn-solve g 3 hp B3 1) (quote none)) #f (sf-equal? (mxn-solve g 3 hp B3 1) Amix)))

(display "the n = 2 specialization reproduces the sqrt field (g = x): INT (...) exp(x) = sqrt(x) exp(x):") (newline)
(define gx (list 0 1))   ; g = x, so y = sqrt x
(define Ays (sf-y 2))
(define Bs (mxn-rhs gx 2 hp Ays))
(chk "n=2 sqrt field: solver recovers A = sqrt x (this module subsumes mixedexp)" (if (equal? (mxn-solve gx 2 hp Bs 0) (quote none)) #f (sf-equal? (mxn-solve gx 2 hp Bs 0) Ays)))

(newline)
(display "Mixed exp over y^n = g for any n: the exponential decouples the y-power sectors into independent Risch") (newline)
(display "differential equations over Q(x), each solved and the assembled answer certified inside the field.") (newline)
