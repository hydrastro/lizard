; RUNG 5: MIXED LOGARITHMIC-OVER-ALGEBRAIC integration over the GENERAL superelliptic field K = Q(x)[y]/(y^n - g)
; for arbitrary n -- the companion of the general-n exponential case (mixedexpn.lisp), lifting the n=2
; logarithmic case (mixedlog.lisp) to any degree using sefield.lisp (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT (P_1 t + P_0) dx where t = log(h) (t' = h'/h) and P_1, P_0 are field elements of K.  An
; element of the tower K(t) is a list of field elements (C_0 C_1 ... C_d) = sum_i C_i t^i.  The derivation is
; d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i, with C_i' the sefield derivative.  Integrating a
; degree-1 input gives Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0, 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0; the
; field-coefficient sectors decouple within each t-degree, so it is one exact linear system, certified by
; differentiating in K(t).
(import "cas/mixedlogn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Mixed log-over-algebraic over y^n = g for general n: INT (P_1 log h + P_0) dx, solved in the tower K(t).") (newline) (newline)

(define g (list 1 0 0 1))        ; g = x^3 + 1, the cube-root curve y^3 = x^3 + 1
(define tp (rat-make (list 1) (list 0 1)))   ; t' = 1/x, i.e. t = log x
(define y (sf-y 3))

(display "the cube-root case INT ((x^2/(x^3+1)) y log x + (1/x) y) dx = y log x = (x^3+1)^(1/3) log x:") (newline)
(define Q (list (sf-zeros 3) y))   ; Q = y t
(define B (mln-deriv g 3 tp Q))
(display "  d/dx(y log x): t^1 coeff ") (display (mln-nth B 1)) (display " (= (x^2/(x^3+1)) y), t^0 coeff ") (display (mln-nth B 0)) (display " (= y/x)") (newline)
(chk "the constructive identity INT B dx = y log x certifies in K(t)" (mln-certify g 3 tp Q B))

(display "solving the tower system -- recover Q from B:") (newline)
(define Qsol (mln-solve g 3 tp B 0))
(display "  mln-solve recovers Q = ") (display Qsol) (display "  = y log x") (newline)
(chk "the solver recovers Q = y . log x" (if (equal? Qsol (quote none)) #f (mln-eq? g 3 Qsol Q)))
(define res (mln-integrate g 3 tp B 1))
(chk "the top-level integrator returns the elementary answer" (equal? (car res) (quote elementary)))

(display "the genuine t^2 case, INT (log x)/x dx = (1/2)(log x)^2 (with constant-in-y coefficient):") (newline)
(define Qt2 (list (sf-zeros 3) (sf-zeros 3) (sf-set (sf-zeros 3) 0 (rat-make (list 1) (list 2)))))   ; (1/2) t^2
(define Bt2 (mln-deriv g 3 tp Qt2))
(chk "the solver recovers Q = (1/2)(log x)^2 (answer one t-degree higher)" (if (equal? (mln-solve g 3 tp Bt2 0) (quote none)) #f (mln-eq? g 3 (mln-solve g 3 tp Bt2 0) Qt2)))

(display "the y^2 sector, INT (...) dx = y^2 log x:") (newline)
(define Qy2 (list (sf-zeros 3) (sf-set (sf-zeros 3) 2 (rat-one))))   ; y^2 t
(define By2 (mln-deriv g 3 tp Qy2))
(chk "the solver recovers Q = y^2 . log x" (if (equal? (mln-solve g 3 tp By2 0) (quote none)) #f (mln-eq? g 3 (mln-solve g 3 tp By2 0) Qy2)))

(display "the n = 2 specialization reproduces the sqrt field: INT (...) dx = sqrt(x) log x:") (newline)
(define gx (list 0 1))   ; g = x, so y = sqrt x
(define ys (sf-y 2))
(define Qs (list (sf-zeros 2) ys))   ; sqrt(x) log x
(define Bs (mln-deriv gx 2 tp Qs))
(chk "n=2 sqrt field: solver recovers Q = sqrt(x) log x (this module subsumes mixedlog)" (if (equal? (mln-solve gx 2 tp Bs 0) (quote none)) #f (mln-eq? gx 2 (mln-solve gx 2 tp Bs 0) Qs)))

(newline)
(display "Mixed log over y^n = g for any n: a primitive monomial log(h) integrated over an algebraic coefficient") (newline)
(display "field, the tower system Q_2 t^2 + Q_1 t + Q_0 solved inside K = Q(x)[y]/(y^n - g), every answer certified.") (newline)
