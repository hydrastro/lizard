; integral-cert.lisp — proof-carrying INTEGRATION, via the Fundamental Theorem
; of Calculus used as a *verification* strategy.
;
; The trusted kernel already has a derivative judgment  Der f g  ("g is the
; derivative of f"), with the differentiation rules postulated as its
; constructors (see cas/diff-cert.lisp).  The observation that makes integration
; free of new trust:
;
;     "F is an antiderivative of f"   IS EXACTLY   Der F f.
;
; So to certify  ∫ f dx = F  we exhibit a term of type  Der F f  — the very same
; objects the differentiator builds.  The integrator itself (finding F) is
; UNTRUSTED search; the kernel is what makes the answer trustworthy.  Claim a
; wrong antiderivative and no term inhabits the type — the kernel rejects it.
;
; Honest scope: the elementary fragment whose derivative rules carry NO rational
; coefficients — 1/x↦ln, cos↦sin, exp↦exp, the constant 1↦x, (-sin)↦cos — closed
; under finite SUMS (linearity, via der_add).  The polynomial power rule
;   ∫ xⁿ dx = xⁿ⁺¹/(n+1)
; is deliberately NOT included: certifying it requires the kernel to compute
; (n+1)·(1/(n+1)) = 1, i.e. rational arithmetic inside the trusted kernel — a
; soundness-critical extension we are not making here.

(import "cas/diff-cert.lisp" :as dc)

; ---- the integrand as a kernel function term (R -> R) ------------------------
; descriptors:  'cos 'exp 'recip 'one 'negsin   and   (+ d1 d2 ...)
(define (ig-fn d)
  (cond ((equal? d 'cos)    'cos)
        ((equal? d 'exp)    'exp)
        ((equal? d 'recip)  'recip)
        ((equal? d 'one)    (dc:fn 'oneR))
        ((equal? d 'negsin) (dc:fn (dc:kapp 'neg (dc:kapp 'sin 'x))))
        ((and (pair? d) (equal? (car d) '+))
         (dc:fn (dc:k2 'add (dc:kapp (ig-fn (car (cdr d))) 'x)
                            (dc:kapp (ig-fn (car (cdr (cdr d)))) 'x))))
        (else d)))

; ---- the integrator: integrate d -> (list F proof),  proof : Der F (ig-fn d) -
; Each base case names the antiderivative F and the rule that proves it; the sum
; case glues two certificates with der_add (linearity of the integral).
(define (integrate d)
  (cond ((equal? d 'cos)    (list 'sin 'der_sin))     ; ∫cos  = sin   (der_sin)
        ((equal? d 'exp)    (list 'exp 'der_exp))     ; ∫exp  = exp   (der_exp)
        ((equal? d 'recip)  (list 'ln  'der_ln))      ; ∫1/x  = ln    (der_ln)
        ((equal? d 'one)    (list (dc:fn 'x) 'der_id)); ∫1    = x     (der_id)
        ((equal? d 'negsin) (list 'cos 'der_cos))     ; ∫-sin = cos   (der_cos)
        ((and (pair? d) (equal? (car d) '+))
         (let ((ra (integrate (car (cdr d))))
               (rb (integrate (car (cdr (cdr d))))))
           (let ((f1 (car ra)) (p1 (car (cdr ra)))
                 (f2 (car rb)) (p2 (car (cdr rb))))
             ; antiderivative of a sum is the (pointwise) sum of antiderivatives
             (list (dc:fn (dc:k2 'add (dc:kapp f1 'x) (dc:kapp f2 'x)))
                   ; der_add F1 (ig a) F2 (ig b) p1 p2
                   ;   : Der (λx.F1 x + F2 x) (λx.(ig a) x + (ig b) x)
                   (dc:kapp (dc:kapp
                              (dc:k6 'der_add f1 (ig-fn (car (cdr d)))
                                              f2 (ig-fn (car (cdr (cdr d)))))
                              p1) p2)))))
        (else (list d 'der_id))))   ; unknown integrand: certificate will fail

; the antiderivative term alone
(define (antiderivative d) (car (integrate d)))

; ---- the trust boundary: verify d -> #t iff the kernel ACCEPTS the certificate
; as a proof that the antiderivative really differentiates back to the integrand.
(define (verify d)
  (let ((r (integrate d)))
    (kernel-check (car (cdr r)) (dc:Der (car r) (ig-fn d)))))

; convenience: does the kernel accept the (possibly wrong) claim  ∫ d dx = Fterm ?
; used to demonstrate that a WRONG antiderivative is rejected.
(define (claim-holds? d Fterm proof)
  (kernel-check proof (dc:Der Fterm (ig-fn d))))
