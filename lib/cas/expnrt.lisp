; -*- lisp -*-
; lib/cas/expnrt.lisp -- the exponential proper case: integration of a proper rational function of
; theta = e^x (denominator coprime to theta), the mirror of towerrt.lisp's logarithmic case.
;
; The residue criterion is the same -- INT a/d is elementary iff R(z) = Res_theta(d, a - z Dd) has
; constant roots, with INT a/d = sum_i c_i log gcd_theta(d, a - c_i Dd) -- but the exponential
; derivation differs structurally: D(theta^i) = i theta^i, so Dd has the SAME theta-degree as d, and
; each logarithm argument v_i ~ theta^{deg v_i} behaves like (deg v_i) x at infinity (because
; log theta = x).  The honest answer therefore carries a base-field correction: an integral whose
; logarithmic part is sum_i c_i log(v_i) really equals sum_i c_i log(v_i) - (sum_i c_i deg_theta v_i) x,
; the subtracted multiple of x cancelling the spurious constant the logarithms introduce.  Hermite
; reduction (over the same monomial) handles the rational part, the residue reduction (reused from
; towerrt.lisp) the logarithms, and the correction is read off from the residues and argument
; degrees.  Every answer is certified by differentiating in the tower.  It finds, for instance,
; INT (-5 e^x - 6)/(e^(2x) + 3 e^x + 2) dx = log(e^x+1) + 2 log(e^x+2) - 3x, and the proper part of
; e^x/(e^x+1), namely INT -1/(e^x+1) dx = log(e^x+1) - x.  Builds on towerrt.lisp.

(import "cas/towerrt.lisp")

(define (exn-mono) (list 'exp (list 0 1)))                       ; theta = e^x, D theta = theta

; correction magnitude:  sum_i c_i deg_theta(v_i)
(define (exn-corr terms acc)
  (if (null? terms) acc (exn-corr (cdr terms) (+ acc (* (car (car terms)) (rfpoly-deg (cdr (car terms))))))))

; INT A/D, A/D proper in theta=e^x with D coprime to theta
;   -> (list 'ok g-tr terms corr) | (list 'algebraic) | (list 'non-elementary)
;   g-tr rational part, terms = list of (c . v), corr = the coefficient of -x in the answer
(define (int-exp-rational A D)
  (let ((H (hermite A D (exn-mono))))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '() 0)
          (let ((rt (trt-logpart as ds (exn-mono))))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt)) (exn-corr (car (cdr rt)) 0)) rt))))))

; tr for the constant -corr (the derivative of the -corr*x correction term)
(define (exn-negcorr-tr corr) (list (rf-const (rat-from-poly (list (- 0 corr)))) (rf-const (rat-one))))

; certificate:  D(g) + sum c Dv/v - corr  =  A/D
(define (int-exp-rational-verify A D)
  (let ((r (int-exp-rational A D)))
    (if (equal? (car r) 'ok)
        (tr-equal? (tr-add (trt-add-logderivs (tr-deriv (car (cdr r)) (exn-mono)) (car (cdr (cdr r))) (exn-mono))
                           (exn-negcorr-tr (car (cdr (cdr (cdr r))))))
                   (list A D))
        #f)))
(define (int-exp-rational-elementary? A D) (equal? (car (int-exp-rational A D)) 'ok))
