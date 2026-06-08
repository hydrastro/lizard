; -*- lisp -*-
; lib/cas/rischratmonolog.lisp -- integration of rational functions of log x, the dual of the e^x case, and a
; genuine DECIDABILITY result.  Unlike e^x, the logarithm has no rational derivative-relation: under t = log x
; (so x = e^t, dx = e^t dt) one gets INT R(log x) dx = INT R(t) e^t dt, which is an EXPONENTIAL integrand, not a
; rational one.  So the log case does NOT collapse to the rational integrator the way INT R(e^x) did; it is
; decided by the exponential Liouville machinery (docs/TRAGER_ROADMAP.md, the summit -- the "any rational function
; of log x" row, and the asymmetry that makes it a decidability statement rather than a closed form).
;
; The split:
;   * R a POLYNOMIAL in log x: INT P(log x) dx is elementary, and is integrated directly through the logarithmic
;     tower (te-integrate over the (log x) level), certified by differentiation.
;   * R a PROPER RATIONAL in log x (a genuine pole in the log-monomial): INT R(t) e^t dt is the
;     exponential-integral situation, decided by the rational-coefficient exponential Liouville decider
;     (liouvillerat: INT (poly + sum r_j/t^j) e^t dt is elementary iff a rational S with S' + S = R exists).
;     A nonzero residue r_1 is the Ei obstruction -- so INT 1/(log x) dx (the logarithmic integral li) is PROVEN
;     non-elementary, exactly as INT e^t/t dt = Ei(t) is.
;
; This is the honest realization of the row: the elementary sub-case is integrated and certified, and the
; non-elementary sub-case is PROVEN non-elementary by the exact Ei obstruction -- a decision, not a failure.
;
; Public:
;   ratmonolog-poly-integrate Pcoeffs -> (list 'elementary y) | ... : INT P(log x) dx via the log tower (Pcoeffs
;       low-to-high in theta = log x), certified
;   ratmonolog-decide poly r-prin     -> (list 'elementary ..) | (list 'non-elementary ..) : decide INT R(log x)
;       where R = poly(log x) + sum r_j/(log x)^j, via the substituted INT R(t) e^t dt and the exponential decider
;   ratmonolog-li-is-nonelementary    -> #t : the worked fact that INT 1/log x dx (li) is non-elementary
;
; Verified: INT (log x)^2 = x(log x)^2 - 2x log x + 2x and INT (log x)^3 elementary through the tower, certified;
; INT 1/log x (li) proven non-elementary; INT 1/(log x)^2 proven non-elementary; a polynomial-only R decided
; elementary by the substituted exponential decider, consistent with the tower.
;
; Builds on rischintn.lisp (the log tower integrator), liouvillerat.lisp (the exponential Ei decider), and
; tower.lisp / poly.lisp.

(import "cas/rischintn.lisp")
(import "cas/liouvillerat.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; ----- the logarithmic tower level theta = log x -----
(define (rml-logtower) (list (list (quote log) (rat-from-poly (list 0 1)))))

; ----- polynomial in log x: integrate directly through the tower, certified -----
(define (ratmonolog-poly-integrate Pcoeffs) (te-integrate (rml-logtower) 1 (rml-ratify Pcoeffs)))
; lift a list of plain rational-number/poly coeffs into tower base elements (rationals)
(define (rml-ratify cs) (rml-rat-go cs))
(define (rml-rat-go cs) (if (null? cs) (quote ()) (cons (rml-one cs) (rml-rat-go (cdr cs)))))
(define (rml-one cs) (if (pair? (car cs)) (car cs) (rat-from-poly (list (car cs)))))
(define (ratmonolog-poly-certify Pcoeffs y) (te-int-certify (rml-logtower) 1 (rml-ratify Pcoeffs) y))

; ----- general rational in log x: decide via INT R(t) e^t dt (the substitution t = log x, dx = e^t dt) -----
; R = poly(t) + sum_j r_j / t^j; this is exactly the input shape of the exponential Liouville decider lr-decide.
(define (ratmonolog-decide poly r-prin) (lr-decide poly r-prin))

; ----- the worked fact: INT 1/log x dx = li(x) is non-elementary (residue r_1 = 1, the Ei obstruction) -----
(define (ratmonolog-li-is-nonelementary) (equal? (car (lr-decide (quote ()) (list 1))) (quote non-elementary)))
