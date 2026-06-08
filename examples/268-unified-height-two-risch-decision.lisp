; THE unified height-two transcendental Risch DECISION driver.  Until now the height-two tower machinery
; was a toolbox the caller dispatched by hand (int-h2 / int-h2-full for a primitive second monomial,
; t2e-hermite + t2e-int-powersum + t2e-int-rde for an exponential one).  h2-integrate is the single entry
; point the Risch decision procedure is meant to be: given any height-two integrand A/D over a tower with a
; second monomial theta2, it splits A/D into its polynomial and proper parts, drives each through the right
; machinery, and returns EITHER an elementary antiderivative (certified by differentiation) OR a PROOF that
; none exists.  The decisive demonstration is the pair below: it finds the answer to INT e^x exp(e^x) dx and
; certifies it, and it PROVES that INT exp(e^x) dx has no elementary antiderivative (the Risch differential
; equation has no solution) -- a proof that no answer exists, not a failure to find one.
(import "cas/tower2risch.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))

(define MEXP (list (quote exp) (list 0 1)))                                  ; theta1 = exp(x)
(define uprime (list (list (rat-zero) (rat-one)) (list (rat-one))))          ; theta2 = exp(exp x): D theta2 = uprime * theta2
(define W1 (t2-trrat 1))                                                      ; primitive theta2: D theta2 = 1

(display "Driver = h2-integrate(A, D, kind, w, mono1) -> (elementary <ans> ...) | (non-elementary <reason>)") (newline) (newline)

; ---------- exponential second monomial ----------
(display "exponential tower, theta2 = exp(exp x):") (newline)
; INT e^x exp(exp x) dx = exp(exp x)  -- ELEMENTARY, found and certified
(define Aelem (list (k1-zero) (list (list (rat-zero) (rat-one)) (list (rat-one)))))   ; (e^x) theta2
(display "  INT e^x exp(exp x) dx  -> ") (display (h2int-status (h2-integrate Aelem (list (k1-one)) (quote exp) uprime MEXP))) (newline)
(must "  elementary answer found AND certified by differentiation" (h2-integrate-verify Aelem (list (k1-one)) (quote exp) uprime MEXP))
; INT exp(exp x) dx -- NON-ELEMENTARY, proven
(define Anon (list (k1-zero) (k1-one)))                                       ; theta2
(define rnon (h2-integrate Anon (list (k1-one)) (quote exp) uprime MEXP))
(display "  INT exp(exp x) dx      -> ") (display (h2int-status rnon)) (newline)
(display "      reason: ") (display (car (cdr rnon))) (newline)
(must "  INT exp(exp x) dx PROVEN non-elementary (no elementary antiderivative exists)" (equal? (h2int-status rnon) (quote non-elementary)))
(newline)

; ---------- primitive second monomial ----------
(display "primitive tower, theta2 antiderivative-like (D theta2 = 1):") (newline)
; INT 2 theta2 dx = theta2^2 ; INT theta2^2 dx = theta2^3/3 -- ELEMENTARY polynomial parts, certified
(must "  INT 2 theta2 dx = theta2^2 certified" (h2-integrate-verify (h2-monomial (k1-iscale 2 (k1-one)) 1) (list (k1-one)) (quote prim) W1 MEXP))
(must "  INT theta2^2 dx = theta2^3/3 certified" (h2-integrate-verify (h2-monomial (k1-one) 2) (list (k1-one)) (quote prim) W1 MEXP))
(newline)

; ---------- the driver ALWAYS returns a definite verdict ----------
(must "driver decides (certified answer OR non-elementarity proof) on exp elementary input" (h2-integrate-decides? Aelem (list (k1-one)) (quote exp) uprime MEXP))
(must "driver decides on the non-elementary input"                                          (h2-integrate-decides? Anon  (list (k1-one)) (quote exp) uprime MEXP))
(must "driver decides on the primitive input"                                               (h2-integrate-decides? (h2-monomial (k1-one) 2) (list (k1-one)) (quote prim) W1 MEXP))

(newline)
(display "unified height-two Risch driver: one entry point, returns a certified antiderivative or a proof of non-elementarity.") (newline)
