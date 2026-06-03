; Height-two integrals of substitution type, for ANY second monomial -- including the EXPONENTIAL one.
; The chain rule d/dx F(theta2) = F'(theta2) D2(theta2) holds for primitive and exponential theta2 alike,
; so for g in Q(theta2),  INT D2(theta2) g(theta2) dx = [ INT g(t) dt ]_{t = theta2}.  An integrand
; A(theta2)/D(theta2) over K1 is of this type when g = A/(D * D2theta2), reduced over K1[theta2] by an
; exact polynomial gcd, lies in Q(theta2); the integral is then integrate.lisp over Q (logs for linear
; factors, arctangents for irreducible quadratics) with NO resultant over K1.  For theta2 = exp(e^x)
; (D2theta2 = e^x theta2):  INT e^x exp(e^x)/(exp(2 e^x)+1) dx = arctan(exp(e^x)), and
; INT e^x exp(e^x)/(exp(e^x)-1) dx = log(exp(e^x)-1).  The same integrator handles the primitive second
; monomial too.  Certified two ways (compute-once): the reduction A*Dbar = D*D2theta2*Abar is exact in
; K1[theta2], and integrate-verify certifies d/dtheta2 = Abar/Dbar over Q.
(import "cas/tower2sub.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
; certify a PRECOMPUTED result r (no recomputation of the gcd): reduction exact in K1[theta2] AND Q-cert
(define (sub-cert? A D Dth2 r)
  (if (h2-equal? (h2-norm (h2-mul A (t2sub-toh2 (car (cdr (cdr r))))))
                 (h2-norm (h2-mul (h2-mul D Dth2) (t2sub-toh2 (car (cdr r))))))
      (integrate-verify (car (cdr r)) (car (cdr (cdr r))) (car (cdr (cdr (cdr r))))) #f))
(define MEXP (list 'exp (list 0 1)))
(define TH1 (list (list (rat-zero) (rat-one)) (list (rat-one))))         ; theta1 = e^x as K1 element
(define EDth2 (list (k1-zero) TH1))                                      ; D2 theta2 = e^x theta2 = theta1 theta2
; --- EXPONENTIAL arctangent: INT e^x exp(e^x)/(exp(2 e^x)+1) dx = arctan(exp(e^x)) ---
(define EA1 (list (k1-zero) TH1)) (define ED1 (list (k1-from-int 1) (k1-from-int 0) (k1-from-int 1)))
(display "INT e^x exp(e^x)/(exp(2 e^x)+1) dx = arctan(exp(e^x))") (newline)
(define r1 (int-h2-sub EA1 ED1 EDth2 MEXP))                              ; computed ONCE
(define res1 (car (cdr (cdr (cdr r1)))))
(must "reduces to rational integration in theta2 = exp(e^x)" (equal? (car r1) 'ok))
(must "one arctangent term, no logarithms" (if (= (length (acc-arctans (cdr res1))) 1) (= (length (acc-logs (cdr res1))) 0) #f))
(must "CERTIFIED: D2(arctan exp(e^x)) = integrand" (sub-cert? EA1 ED1 EDth2 r1))
; --- EXPONENTIAL logarithm: INT e^x exp(e^x)/(exp(e^x)-1) dx = log(exp(e^x)-1) ---
(define ED2 (list (k1-from-int -1) (k1-from-int 1)))                     ; theta2 - 1
(display "INT e^x exp(e^x)/(exp(e^x)-1) dx = log(exp(e^x)-1)") (newline)
(define r2 (int-h2-sub EA1 ED2 EDth2 MEXP))                              ; computed ONCE
(define res2 (car (cdr (cdr (cdr r2)))))
(must "one logarithm term" (= (length (acc-logs (cdr res2))) 1))
(must "CERTIFIED: D2(log(exp(e^x)-1)) = integrand" (sub-cert? EA1 ED2 EDth2 r2))
(newline) (display "height-two EXPONENTIAL substitution integrals certified (arctangent and logarithm), no resultant.") (newline)
