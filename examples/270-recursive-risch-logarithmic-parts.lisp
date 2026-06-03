; The recursive Risch driver, EXTENDED to carry logarithmic parts through the recursion (ntrischlog.lisp).
; ntrisch integrated the polynomial part of a tower element at every level and decided non-elementarity, but it
; discarded the proper-fraction LOGARITHMIC part: its base case treated an integrand whose antiderivative needs
; a logarithm (e.g. INT 1/(x^2-1)) as a dead end.  This module threads a log list through the whole recursion.
;
; Two capabilities are demonstrated:
;   (1) PROPER-FRACTION LOGARITHMIC PARTS at arbitrary depth.  A proper fraction N/V of the top monomial whose
;       numerator is a constant multiple of D(V) integrates to c log(V); the argument V is a tower element at the
;       current level, so the answer slots straight into the recursion -- shown here through depth 3.
;   (2) NESTED LOGARITHMS.  A new logarithm log(u) whose argument u is itself a lower tower element is just a
;       log term produced by the same recognizer.  INT 1/(x log x) dx = log(log x) is exactly INT (D theta1)/theta1
;       with theta1 = log x, so the driver returns the nested logarithm log(log x), certified.
;
; The answer type is (elementary RAT LOGS) with LOGS a list of (coeff arg) meaning coeff*log(arg).  Every answer
; is certified by the cleared logarithmic identity  D(RAT)*V + sum_i coeff_i (D arg_i)(V/arg_i) = numerator * V,
; checked by tower arithmetic at the current level -- so a returned 'elementary is always certified, and inputs
; whose residues are not constants are reported 'not-handled rather than forced.
(import "cas/ntrischlog.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))

(define t1 (nt-monomial 1 (rat-one) 1))
(define one1 (nt-lift 1 (rat-one)))
(define se1 (list (list (quote exp) (rat-one))))                                 ; theta1 = e^x
(define se2 (list (list (quote exp) (rat-one)) (list (quote exp) t1)))           ; theta2 = exp(e^x)
(define se3 (list (list (quote exp) (rat-one)) (list (quote exp) t1)
                  (list (quote exp) (nt-monomial 2 (nt-lift 1 (rat-one)) 1))))   ; theta3 = exp(exp(e^x))
(define t2 (nt-monomial 2 (nt-lift 1 (rat-one)) 1))
(define t3 (nt-monomial 3 (nt-lift 2 (rat-one)) 1))
(define dlogx (rat-make (list 1) (list 0 1)))                                    ; 1/x
(define sL (list (list (quote prim) dlogx)))                                     ; theta1 = log x

(display "Logs carried through the recursion: answer = rational part + sum of c*log(arg), certified at every level.") (newline) (newline)

; ===== (0) base-field logarithms are no longer discarded =====
(display "base field Q(x):") (newline)
(must "INT 1/(x^2-1) dx = (1/2) log((x+1)/(x-1))            certified" 
      (let ((p (rat-make (list 1) (list -1 0 1)))) (ntl-verify 0 (quote ()) p (ntl-integrate 0 (quote ()) p))))

; ===== (1) proper-fraction logarithmic parts at increasing depth =====
(display "proper-fraction logarithmic part (single rational residue), lifted into the recursion:") (newline)
(must "INT e^x/(e^x+1) dx = log(e^x+1)                       certified  [depth 1]"
      (ntl-verify-frac 1 se1 t1 (nt-add 1 t1 one1) (ntl-integrate-frac 1 se1 t1 (nt-add 1 t1 one1))))
(must "INT 2e^x/(e^x+1) dx = 2 log(e^x+1)   (residue 2)      certified  [depth 1]"
      (ntl-verify-frac 1 se1 (nt-iscale 1 2 t1) (nt-add 1 t1 one1) (ntl-integrate-frac 1 se1 (nt-iscale 1 2 t1) (nt-add 1 t1 one1))))
(must "INT (D t2)/(t2+1) dx = log(exp(e^x)+1)                certified  [depth 2]"
      (ntl-verify-frac 2 se2 (nt-deriv 2 se2 t2) (nt-add 2 t2 (nt-lift 2 (rat-one))) (ntl-integrate-frac 2 se2 (nt-deriv 2 se2 t2) (nt-add 2 t2 (nt-lift 2 (rat-one))))))
(must "INT (D t3)/(t3+1) dx = log(exp(exp(e^x))+1)           certified  [depth 3]"
      (ntl-verify-frac 3 se3 (nt-deriv 3 se3 t3) (nt-add 3 t3 (nt-lift 3 (rat-one))) (ntl-integrate-frac 3 se3 (nt-deriv 3 se3 t3) (nt-add 3 t3 (nt-lift 3 (rat-one))))))

; ===== (2) nested logarithms =====
(display "nested logarithms (a new log whose argument is a lower tower element):") (newline)
(define r_ll (ntl-integrate-frac 1 sL (nt-deriv 1 sL t1) t1))
(display "  INT 1/(x log x) dx -> ") (display (car r_ll)) (display ", logs = ") (display (ntl-logs r_ll)) (newline)
(must "INT 1/(x log x) dx = log(log x)                       certified" (ntl-verify-frac 1 sL (nt-deriv 1 sL t1) t1 r_ll))
(must "INT 1/(x (log x + 1)) dx = log(log x + 1)             certified"
      (ntl-verify-frac 1 sL (nt-deriv 1 sL t1) (nt-add 1 t1 one1) (ntl-integrate-frac 1 sL (nt-deriv 1 sL t1) (nt-add 1 t1 one1))))
(must "INT 2e^(2x)/(e^(2x)+1) dx = log(e^(2x)+1)             certified"
      (let ((u (nt-add 1 (nt-monomial 1 (rat-one) 2) one1))) (ntl-verify-frac 1 se1 (nt-deriv 1 se1 u) u (ntl-integrate-frac 1 se1 (nt-deriv 1 se1 u) u))))

; ===== soundness: a fraction that is NOT a logarithmic derivative is honestly not-handled =====
(display "soundness (no false positives):") (newline)
(define r_neg (ntl-integrate-frac 2 se2 (nt-lift 2 (rat-one)) (nt-add 2 t2 (nt-lift 2 (rat-one)))))
(display "  INT 1/(exp(e^x)+1) dx -> ") (display (car r_neg)) (display "  (correctly NOT claimed elementary)") (newline)
(must "  non-logarithmic-derivative fraction reported not-handled, not a false answer" (not (ntl-elem? r_neg)))

(newline)
(display "recursive Risch driver now carries logarithms: proper-fraction RootSum logs at depth, and nested logs, all certified.") (newline)
