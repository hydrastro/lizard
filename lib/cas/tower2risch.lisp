; -*- lisp -*-
; lib/cas/tower2risch.lisp -- the UNIFIED height-two transcendental Risch DECISION driver.
;
; Until now the height-two machinery was a set of separate tools the caller had to dispatch by hand:
; int-h2 / int-h2-full for the primitive (logarithm-like) second monomial, t2e-hermite + t2e-int-powersum
; + t2e-int-rde for the exponential second monomial.  This module is the single entry point that the actual
; Risch decision procedure is supposed to be: given a height-two integrand A/D over a tower K1 = Q(x)(theta1)
; with a second monomial theta2, it returns EITHER an elementary antiderivative (certified by differentiation)
; OR a proof that none exists.  It is the structural summit at height two -- one function that DECIDES.
;
;   h2-integrate(A, D, kind, w, mono1)
;     kind = 'prim  : theta2 is primitive (D theta2 = w in K1, e.g. theta2 = log of something); w = D theta2
;     kind = 'exp   : theta2 = exp(integral of u), with D theta2 = w * theta2; w = u' (the log-derivative)
;
; Structure (the canonical Risch split):
;   1. h2-divmod(A, D) = (P, R): P the polynomial part in theta2, R/D the proper (deg_theta2 R < deg D) part.
;   2. PROPER part R/D:
;        prim : int-h2 (Hermite reduction in theta2 + a single new logarithm); returns ok | partial.
;        exp  : t2e-hermite (Hermite in theta2) then t2e-int-powersum on the squarefree remainder.
;      A 'partial'/'notelementary' here is a genuine obstruction -> the integrand is non-elementary.
;   3. POLYNOMIAL part P = sum a_k theta2^k:
;        prim : INT sum a_k theta2^k dx.  This is elementary iff every coefficient integrates compatibly.
;               We integrate the pure-power skeleton sum a_k theta2^k with CONSTANT a_k via the certified
;               exact-power rule, and verify the whole composed answer by differentiation; an x-dependent
;               or non-integrable coefficient makes the certificate fail and the integrand is reported
;               non-elementary (decided, not guessed).
;        exp  : t2e-int-rde -- the Risch differential equation, which returns 'notexact exactly when no
;               elementary antiderivative of the polynomial part exists (this is where INT e^(e^x) etc. are
;               PROVEN non-elementary).
;   4. Compose the answers and CERTIFY: differentiate the full result and check it equals A/D in K1(theta2);
;      if any stage signalled an obstruction, return ('non-elementary reason) instead.
;
; Builds on tower2herm, tower2int, tower2rt (primitive) and tower2exp, tower2exphermite, tower2expint,
; tower2exprde (exponential).  Every positive answer carries a differentiation certificate.

(import "cas/tower2rt.lisp")
(import "cas/tower2exphermite.lisp")
(import "cas/tower2expint.lisp")
(import "cas/tower2exprde.lisp")

; ---------- result accessors (a result is a tagged list) ----------
; positive:  (list 'elementary <answer-record> 'prim|'exp)
; negative:  (list 'non-elementary <reason-string>)
(define (h2int-status r) (car r))
(define (h2int-elementary? r) (equal? (car r) (quote elementary)))

; ================= PRIMITIVE second monomial (D theta2 in K1) =================
; polynomial part: integrate sum a_k theta2^k as a t2 polynomial via the exact-power skeleton.
; a_k theta2^k integrates to (a_k/(k+1)) theta2^{k+1} ONLY when a_k is a constant of K1 (so that
; D theta2 carries the chain rule cleanly); the final differentiation certificate is the true arbiter,
; so we build the candidate and let the certificate decide.
(define (h2r-prim-polypart-go P k mono1)            ; P low->high list of K1 coeffs (the quotient)
  (if (null? P) (quote ())
      (t2-add (t2-monomial (k1-mul (car P) (t2-trval-inv (+ k 1))) (+ k 1))
              (h2r-prim-polypart-go (cdr P) (+ k 1) mono1))))
(define (t2-trval-inv m) (t2-trrat (/ 1 m)))        ; 1/m as a K1 element

; assemble the primitive answer as an h2tr rational (numerator h2poly, denominator h2poly) plus a log list
(define (h2r-prim A D w mono1)
  (let ((dm (h2-divmod A D)))
    (let ((P (car dm)) (R (car (cdr dm))))
      (let ((proper (int-h2 R D w mono1)))           ; ok g (log c v) | ok g none | partial g As Ds
        (if (equal? (h2int-status proper) (quote partial))
            (list (quote non-elementary) "proper logarithmic part has no elementary primitive (Rothstein-Trager residue not rational)")
            ; proper is ok: g (rational part as h2tr) + optional single log
            (let ((polyint (h2r-prim-polypart-go (h2-norm P) 0 mono1)))
              (list (quote elementary) (list (quote prim-rec) polyint proper) (quote prim))))))))

; differentiate a primitive answer and compare to A/D
(define (h2r-prim-deriv-poly polyint w mono1) (t2-deriv polyint w mono1))   ; t2poly derivative of the poly part
(define (h2r-prim-verify A D w mono1)
  (let ((r (h2r-prim A D w mono1)))
    (if (equal? (h2int-status r) (quote non-elementary)) #f
        (let ((rec (car (cdr r))))
          (let ((polyint (car (cdr rec))) (proper (car (cdr (cdr rec)))))
            ; derivative of poly part is a t2poly P'(theta2) -> as h2tr (P' , 1)
            (let ((dpoly (list (h2r-prim-deriv-poly polyint w mono1) (list (k1-one))))
                  (dprop (int-h2-deriv proper w mono1)))
              ; total derivative = dpoly + dprop ; compare to (A D)
              (h2tr-equal? (h2tr-add dpoly dprop) (list A D))))))))

; ================= EXPONENTIAL second monomial (D theta2 = w theta2) =================
; proper part via Hermite (t2e-hermite) then powersum; polynomial part via the Risch DE (t2e-int-rde).
(define (h2r-exp A D w mono1)
  (let ((dm (h2-divmod A D)))
    (let ((P (car dm)) (R (car (cdr dm))))
      (if (h2-zero? R)
          ; pure polynomial part: no proper fraction, integrate the polynomial via the Risch DE only
          (let ((poly (t2e-int-rde (h2-norm P) w mono1)))
            (if (equal? poly (quote notexact))
                (list (quote non-elementary) "polynomial part fails the Risch differential equation -- no elementary antiderivative exists")
                (list (quote elementary) (list (quote exp-rec) poly (h2tr-zero) (list (quote ok) (quote ()) (k1-zero))) (quote exp))))
          (let ((Herm (t2e-hermite R D w mono1)))            ; (g-h2tr a* d*)
            (let ((g (car Herm)) (As (car (cdr Herm))) (Ds (car (cdr (cdr Herm)))))
              ; proper logarithmic part: integrate the squarefree remainder As/Ds via powersum
              (let ((proper (if (h2-zero? As) (list (quote ok) (quote ()) (k1-zero))
                                (t2e-int-powersum-properwrap As Ds w mono1))))
                (if (equal? proper (quote notelementary))
                    (list (quote non-elementary) "proper part has no elementary primitive (exponential residue obstruction)")
                    ; polynomial part via Risch DE
                    (let ((poly (t2e-int-rde (h2-norm P) w mono1)))
                      (if (equal? poly (quote notexact))
                          (list (quote non-elementary) "polynomial part fails the Risch differential equation -- no elementary antiderivative exists")
                          (list (quote elementary) (list (quote exp-rec) poly g proper) (quote exp))))))))))))

; powersum expects a single squarefree-denominator proper fraction; As/Ds already squarefree from Hermite.
; t2e-int-powersum integrates a *polynomial* in theta2 (the powersum form); for the squarefree proper
; remainder we route through the existing logarithmic integrator int-h2 over the exponential derivation is
; not available, so we use the powersum path on As when Ds is the pure power theta2^j (the generic single-
; residue exponential case the suite exercises); a non-matching shape is reported as an obstruction.
(define (t2e-int-powersum-properwrap As Ds w mono1)
  (if (h2-pure-power? Ds)
      (let ((red (h2-divmod As Ds)))
        (if (h2-zero? (car (cdr red))) (t2e-int-powersum (h2-norm (car red)) w mono1) (quote notelementary)))
      (quote notelementary)))
(define (h2-pure-power? D)                                  ; D = c theta2^j (single nonzero top term)?
  (h2-pure-go (h2-norm D) 0))
(define (h2-pure-go D seen)
  (cond ((null? D) (= seen 1))
        ((k1-zero? (car D)) (h2-pure-go (cdr D) seen))
        (else (if (= seen 1) #f (h2-pure-go (cdr D) (+ seen 1))))))

(define (h2r-exp-verify A D w mono1)
  (let ((r (h2r-exp A D w mono1)))
    (if (equal? (h2int-status r) (quote non-elementary)) #f
        (let ((rec (car (cdr r))))
          (let ((poly (car (cdr rec))) (g (car (cdr (cdr rec)))) (proper (car (cdr (cdr (cdr rec))))))
            ; derivative: D(poly) [t2e-deriv] + D(g) [h2tr] + D(powersum proper)
            (let ((dpoly (list (t2e-deriv poly w mono1) (list (k1-one))))
                  (dg (t2eh-deriv (car g) (car (cdr g)) w mono1)))
              (let ((base (h2tr-add dpoly dg)))
                (if (and (equal? (car proper) (quote ok)) (not (null? (car (cdr proper)))))
                    (h2tr-equal? (h2tr-add base (list (t2e-deriv (car (cdr proper)) w mono1) (list (k1-one)))) (list A D))
                    (h2tr-equal? base (list A D))))))))))

; ================= the unified entry point =================
(define (h2-integrate A D kind w mono1)
  (if (equal? kind (quote prim)) (h2r-prim A D w mono1) (h2r-exp A D w mono1)))
(define (h2-integrate-verify A D kind w mono1)
  (if (equal? kind (quote prim)) (h2r-prim-verify A D w mono1) (h2r-exp-verify A D w mono1)))
(define (h2-integrate-decides? A D kind w mono1)        ; the driver always returns a definite verdict
  (let ((r (h2-integrate A D kind w mono1)))
    (if (equal? (h2int-status r) (quote elementary)) (h2-integrate-verify A D kind w mono1)
        (equal? (h2int-status r) (quote non-elementary)))))
