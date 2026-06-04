; Term-by-term PUISEUX EXPANSION of the branches of a general plane algebraic curve F(x,y) = 0, continuing
; Rung 4 of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md).  newton.lisp gives each branch's leading
; exponent and coefficient (the Newton-polygon edges); this module (puiseuxg.lisp) iterates from each leading
; term to the FULL Puiseux series -- the local analytic description of an arbitrary algebraic function on which
; the integral basis (the rest of Rung 4) is built.
;
; F is a list of y-coefficients, each a polynomial in x: F = (F0 F1 ... Fd) means sum_j Fj(x) y^j.  A branch of
; Newton slope mu = p/q (reduced) uses the uniformizer t = x^(1/q) and is y = sum_{k>=p} a_k t^k; with a_p the
; chosen edge-polynomial root, every later coefficient is determined LINEARLY by a_k = -[t^{k+L}]F / [t^L]F_y,
; the Newton-Puiseux recurrence for a branch smooth in the uniformizer.  Each branch is returned as
; (puiseux q p coeffs), meaning y = sum_i coeffs[i] x^((p+i)/q), and is power-checked by substitution into F.
(import "cas/puiseuxg.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))
(define (zero? s n) (pg-zero-series? s n))

(display "Full Puiseux series of the branches of F(x,y)=0, power-checked by substitution into F:") (newline) (newline)

(display "a smooth unramified place with two branches, F = y^2 - 1 - x:") (newline)
(define F1 (list (list -1 -1) (list) (list 1)))
(define bs1 (pg-branches F1 5))
(display "  branches y = +-(1 + x/2 - x^2/8 + ...) -> ") (display bs1) (newline)
(chk "two branches y ~ +-1" (= (length bs1) 2))
(chk "branch power-checks (F(x, sqrt(1+x)) = 0)" (zero? (pg-verify F1 (car bs1) 5) 4))

(display "a ramified place (cusp), F = y^2 - x^3:") (newline)
(define Fc (list (list 0 0 0 -1) (list) (list 1)))
(define bsc (pg-branches Fc 6))
(display "  branch y = x^(3/2) in t = x^(1/2) -> ") (display (car bsc)) (newline)
(chk "cusp branch power-checks" (zero? (pg-verify Fc (car bsc) 6) 5))

(display "a genuinely nonlinear general F, F = y - x - y^2 (the branch is the Catalan generating series):") (newline)
(define Fcat (list (list 0 -1) (list 1) (list -1)))
(define bcat (pg-branches Fcat 6))
(display "  y = x + x^2 + 2x^3 + 5x^4 + 14x^5 + 42x^6 + 132x^7 -> ") (display (pg-nth (car bcat) 3)) (newline)
(chk "Catalan coefficients 1 1 2 5 14 42 132" (equal? (pg-nth (car bcat) 3) (list 1 1 2 5 14 42 132)))
(chk "Catalan branch power-checks" (zero? (pg-verify Fcat (car bcat) 6) 5))

(display "two branches of DIFFERENT slope, F = (y - x)(y - x^2) = y^2 - (x + x^2) y + x^3:") (newline)
(define Ft (list (list 0 0 0 1) (list 0 -1 -1) (list 1)))
(define bt (pg-branches Ft 5))
(display "  branches y = x^2 (slope 2) and y = x (slope 1) -> ") (display bt) (newline)
(chk "both branches power-check" (if (zero? (pg-verify Ft (car bt) 5) 4) (zero? (pg-verify Ft (car (cdr bt)) 5) 4) #f))

(display "a node, F = y^2 - x^2 - x^3 (two branches y = +-x*sqrt(1+x)):") (newline)
(define Fn (list (list 0 0 -1 -1) (list) (list 1)))
(define bn (pg-branches Fn 5))
(chk "node: two branches, both power-check" (if (= (length bn) 2) (if (zero? (pg-verify Fn (car bn) 5) 4) (zero? (pg-verify Fn (car (cdr bn)) 5) 4) #f) #f))

(newline)
(display "soundness:") (newline)
(define Fr (list (list -2 -1) (list) (list 1)))
(display "  F = y^2 - 2 - x (leading coeff sqrt(2) irrational) -> ") (display (car (car (pg-branches Fr 4)))) (newline)
(chk "no rational leading coefficient honestly reported needs-radical" (equal? (car (car (pg-branches Fr 4))) (quote needs-radical)))

(newline)
(display "Term-by-term general-F Puiseux working: smooth, ramified, nonlinear, multi-branch, multi-slope, sound.") (newline)
