; -*- lisp -*-
; lib/cas/rischrat.lisp -- complete rational-function integration, the Risch way.
;
; This composes the two halves of the rational case of the Risch algorithm:
;   * Hermite reduction (hermite.lisp) extracts the rational part, leaving an integrand over
;     the squarefree radical of the denominator;
;   * Rothstein-Trager (rothstein.lisp) integrates that remainder as a sum of logarithms.
; Neither step factors the denominator into irreducibles.  The result
;   INT A/D dx = ratnum/ratden + sum_i c_i log(v_i)
; is returned as (ratnum ratden logterms complete?), and is certified by differentiation: the
; reconstructed derivative is checked to equal A/D exactly.  "complete?" is true precisely when
; every residue is rational (the denominator splits over Q); when a factor is irreducible over Q
; its residues are algebraic and contribute an arctangent-type term outside the rational-log
; form, so the full certificate is honestly false there even though the Hermite rational part
; remains exact.  Builds on hermite.lisp and rothstein.lisp.

(import "cas/hermite.lisp")
(import "cas/rothstein.lisp")

(define (ri-cadr l) (car (cdr l)))
(define (ri-caddr l) (car (cdr (cdr l))))
(define (ri-cadddr l) (car (cdr (cdr (cdr l)))))

; INT A/D = ratnum/ratden + sum c_i log v_i   ->  (ratnum ratden logterms complete?)
(define (rat-integrate A D)
  (let ((h (hermite A D)))
    (let ((sn (ri-caddr h)) (sd (ri-cadddr h)))
      (let ((terms (rt-log-part sn sd)))
        (list (car h) (ri-cadr h) terms (= (ros-degsum terms) (poly-deg sd)))))))

(define (rat-integrate-complete? A D) (ri-cadddr (rat-integrate A D)))

; full certificate: d/dx(ratnum/ratden) + sum c_i v_i'/v_i = A/D  (true exactly when complete)
(define (rat-integrate-verify A D)
  (let ((r (rat-integrate A D)))
    (let ((terms (ri-caddr r)))
      (let ((V (ros-prod (ros-args terms))) (dr (hm-ratderiv (car r) (ri-cadr r))))
        (let ((tot (hm-radd (car dr) (cdr dr) (ros-deriv-numer terms V) V)))
          (poly-zero? (poly-sub (poly-mul (car tot) D) (poly-mul A (cdr tot)))))))))

; the Hermite rational part is always exact, even when the log part is algebraic
(define (rat-integrate-rational-ok? A D) (hermite-verify A D))
