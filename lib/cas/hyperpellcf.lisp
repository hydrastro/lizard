; -*- lisp -*-
; lib/cas/hyperpellcf.lisp -- the CF-DRIVEN genus-2 third-kind Pell construction: given ANY hyperelliptic curve
; y^2 = f, find the fundamental unit by the continued fraction of sqrt(f) (polycf) and construct the certified
; third-kind logarithm from it -- generalizing hyperpell past the f = h^2 + c family to every periodic curve,
; including genuine period-2 curves (docs/TRAGER_ROADMAP.md -- the full third-kind construction).
;
; hyperpell built the nonconstant-B third-kind arguments g0^n on the special family f = h^2 + c, where the unit
; g0 = h + y is read off by inspection.  polycf now computes the fundamental unit (A, B) for an arbitrary curve
; whose sqrt(f) is periodic -- so the same power construction applies with g0 = A + B y, no longer requiring f to
; have the h^2 + c shape.  This module is the bridge: it asks polycf for the certified unit, builds g0 = A + B y as
; an algfunc element, and produces INT ((g0^n)'/g0^n) dx = log(g0^n) = n log(g0), each gated by the differentiation
; certificate in the field.  When sqrt(f) is not periodic within the search bound, it reports no-unit (the honest
; bounded negative inherited from polycf), never a forced answer.
;
; Public (f the curve polynomial; B a search bound; n a positive power):
;   hpc-unit f B               -> the certified fundamental unit (list A B) | 'no-unit (from polycf, honest)
;   hpc-has-unit? f B          -> #t iff sqrt(f) is periodic and the unit certifies within bound B
;   hpc-g0 f B                 -> the fundamental unit g0 = A + B y as an algfunc element, or 'no-unit
;   hpc-unit-power f B n       -> g0^n = (A_n . B_n) as polynomials, by field arithmetic, or 'no-unit
;   hpc-logderiv f B n         -> the differential (g0^n)'/g0^n as an algfunc element, or 'no-unit
;   hpc-log-cert f B n         -> #t iff (g0^n) * ((g0^n)'/g0^n) = D(g0^n) in K (the differentiation certificate)
;   hpc-log f B n              -> (list 'log A_n B_n) : INT ((g0^n)'/g0^n) dx = log(A_n + B_n y) = n log(g0), or 'no-unit
;
; Verified: on y^2 = x^6 + x (a genuine period-2 curve, NOT of the form h^2 + c) the CF-found unit drives a
; certified third-kind logarithm and its square; on y^2 = x^6 + 1 (period 1) it agrees with hyperpell; a
; non-periodic curve reports no-unit.
;
; Builds on polycf.lisp (the unit finder) and algfunc.lisp (the genus-agnostic field).

(import "cas/polycf.lisp")
(import "cas/algfunc.lisp")

(define (hpc-pf f) (rat-from-poly f))

; ----- the certified fundamental unit from the continued fraction -----
(define (hpc-unit f B) (hpc-unit-check f (pcf-unit-verified f B)))
(define (hpc-unit-check f u) (if (pair? u) (if (pair? (car u)) u (quote no-unit)) (quote no-unit)))
(define (hpc-has-unit? f B) (if (equal? (hpc-unit f B) (quote no-unit)) #f #t))

; ----- g0 = A + B y as a field element -----
(define (hpc-g0 f B) (hpc-g0-build f (hpc-unit f B)))
(define (hpc-g0-build f u) (if (equal? u (quote no-unit)) (quote no-unit) (af-make (rat-from-poly (car u)) (rat-from-poly (car (cdr u))))))

; ----- powers g0^n and their polynomial (A_n, B_n) -----
(define (hpc-pow f g n) (if (<= n 1) g (af-mul (hpc-pf f) g (hpc-pow f g (- n 1)))))
(define (hpc-gn f B n) (hpc-gn-build f (hpc-g0 f B) n))
(define (hpc-gn-build f g0 n) (if (equal? g0 (quote no-unit)) (quote no-unit) (hpc-pow f g0 n)))
(define (hpc-unit-power f B n) (hpc-up-extract (hpc-gn f B n)))
(define (hpc-up-extract e) (if (equal? e (quote no-unit)) (quote no-unit) (cons (rat-num (af-u e)) (rat-num (af-v e)))))

; ----- the logarithm differential and its certificate -----
(define (hpc-logderiv f B n) (hpc-ld f (hpc-gn f B n)))
(define (hpc-ld f gn) (if (equal? gn (quote no-unit)) (quote no-unit) (af-div (hpc-pf f) (af-deriv (hpc-pf f) gn) gn)))
(define (hpc-log-cert f B n) (hpc-lc f (hpc-gn f B n)))
(define (hpc-lc f gn) (if (equal? gn (quote no-unit)) #f (af-equal? (af-mul (hpc-pf f) gn (af-div (hpc-pf f) (af-deriv (hpc-pf f) gn) gn)) (af-deriv (hpc-pf f) gn))))

; ----- the constructed third-kind logarithm record -----
(define (hpc-log f B n) (hpc-log-build (hpc-unit-power f B n)))
(define (hpc-log-build ab) (if (equal? ab (quote no-unit)) (quote no-unit) (list (quote log) (car ab) (cdr ab))))
