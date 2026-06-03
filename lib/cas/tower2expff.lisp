; -*- lisp -*-
; lib/cas/tower2expff.lisp -- fraction-free Rothstein-Trager logarithmic part for the EXPONENTIAL second
; monomial theta2 = exp(u) with x-dependent integrands.
;
; tower2sub.lisp integrates exponential height-two integrands whose logarithm arguments are constant in x
; (by substitution to trusted rational integration); when the arguments depend on x there is no
; substitution and the residues must come from the Sylvester resultant over K1 = Q(x)(theta1).  The
; entire fraction-free apparatus of tower2ff.lisp -- the Bareiss resultant, the LCM clearing, the gcd-free
; ratio test, the primitive-PRS gcd for the logarithm arguments -- operates on K1[theta2] and is therefore
; INDIFFERENT to which derivation theta2 carries: it only ever consumes D, A and D2(D).  So the exponential
; logarithmic part reuses that apparatus verbatim, feeding it D2(D) computed with the EXPONENTIAL derivation
; D2(theta2) = u' theta2 (t2e-deriv) instead of the primitive one, and certifies with the matching
; exponential logarithmic derivative.  Builds on tower2ff.lisp (machinery) and tower2exp.lisp (derivation).

(import "cas/tower2ff.lisp")
(import "cas/tower2exp.lisp")

; certificate derivative for the exponential case: sum c_i (D2 v_i)/v_i with D2 = t2e-deriv, as one K1 fraction
(define (h2rt-logderiv-exp terms uprime mono1 acc)
  (if (null? terms) acc
      (h2rt-logderiv-exp (cdr terms) uprime mono1
        (h2tr-add acc (list (h2-cscale (k1-from-rat (car (car terms))) (t2e-deriv (car (cdr (car terms))) uprime mono1))
                            (car (cdr (car terms))))))))

; the exponential logarithmic part -- same shape as tower2ff's h2rt-logpart-ff, exponential D2D throughout
(define (h2rt-logpart-exp-ff As Ds uprime mono1)
  (begin (gc)
  (let ((D2D (t2e-deriv Ds uprime mono1)) (N (h2-deg Ds)))
    (let ((cD (ff-denlcm Ds (rf-one))))
      (let ((c (ff-lcm2 (ff-denlcm As (rf-one)) (ff-denlcm D2D (rf-one)))))
        (let ((rs (ff-Rvals (ff-clear-poly Ds cD) (ff-clear-poly As c) (ff-clear-poly D2D c) 0 N (quote ()))))
          (let ((k0 (ff-first-nonzero rs 0)))
            (if (< k0 0) (list (quote degenerate))
                (let ((rats (ff-ratios rs (ff-nth rs k0) (quote ()))))
                  (if (equal? rats (quote notconst)) (list (quote algebraic))
                      (let ((roots (ros-rational-roots (q-lagrange (h2rt-int-list N) rats))))
                        (let ((tt (h2rt-terms-ff roots As Ds D2D uprime mono1 (quote ()) 0)))
                          (if (= (car (cdr tt)) (h2-deg Ds)) (list (quote rootsum) (car tt)) (list (quote algebraic)))))))))))))))

(define (h2rt-logpart-exp-ff-verify As Ds uprime mono1)     ; differentiation certificate (exponential)
  (let ((lg (h2rt-logpart-exp-ff As Ds uprime mono1)))
    (if (equal? (car lg) (quote rootsum))
        (h2tr-equal? (h2rt-logderiv-exp (car (cdr lg)) uprime mono1 (h2tr-zero)) (list As Ds)) #f)))
(define (h2rt-logpart-exp-ff-elementary? As Ds uprime mono1) (equal? (car (h2rt-logpart-exp-ff As Ds uprime mono1)) (quote rootsum)))
