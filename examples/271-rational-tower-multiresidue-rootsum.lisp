; A RATIONAL differential tower (rtower.lisp) and the MULTI-RESIDUE Rothstein-Trager logarithmic part on it.
;
; The uniform tower of ntower/ntrisch kept elements POLYNOMIAL in each monomial, so the coefficient ring at
; each level was not closed under division.  That blocked two things: (a) negative powers and elements rational
; in a monomial (e.g. 1/e^x, 1/log x), and (b) the multi-residue logarithmic part, which needs polynomial gcd
; and resultants over the lower field.  rtower.lisp fixes the representation: at every level the coefficients
; form a genuine FIELD, so an element is a FRACTION of polynomials in the top monomial whose coefficients are
; rational-tower elements one level down, bottoming out at Q(x).
;
; Two capabilities are demonstrated.
;   (1) The recursive DERIVATION on the rational tower handles positive AND negative powers and elements
;       rational in a monomial, at arbitrary depth -- verified here through depth 2, e.g. D(1/exp(e^x)) =
;       -e^x/exp(e^x).  This is the representation boundary the polynomial tower could not cross.
;   (2) The MULTI-RESIDUE logarithmic part: for a proper fraction Pnum/V with V monic squarefree in the top
;       monomial, the residues are the rational roots of the resultant res(V, Pnum - c V'), and each residue c
;       contributes c log(gcd(V, Pnum - c V')).  This yields SEVERAL logarithms at once -- e.g.
;       INT 2e^x/(e^(2x)-1) dx = log(e^x-1) - log(e^x+1) -- at arbitrary depth, certified by differentiation,
;       and it correctly DECLINES integrals whose residues are not rational (complex residues -> not elementary
;       over Q) rather than returning a false answer.
;
; Every elementary RootSum is certified by the cleared identity sum_i c_i (D v_i)(prod_{j!=i} v_j) = Pnum over
; the lower field, and a RootSum is accepted as complete only when the residue-argument degrees sum to deg V;
; otherwise the integral is reported non-elementary.
(import "cas/rtower.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))

; ===== (1) the rational-tower derivation: positive and negative powers, rational-in-monomial =====
(display "rational-tower derivation (the representation boundary crossed):") (newline)
(define s1 (list (list (quote exp) (rat-one))))                       ; theta1 = e^x
(define t1 (rt-theta 1))
(must "D(e^x) = e^x" (rt-equal? 1 (rt-deriv 1 s1 t1) t1))
(must "D(1/e^x) = -1/e^x        (NEGATIVE power, impossible in a polynomial tower)" (rt-equal? 1 (rt-deriv 1 s1 (rt-inv 1 t1)) (rt-neg 1 (rt-inv 1 t1))))
(define sL (list (list (quote prim) (rat-make (list 1) (list 0 1)))))  ; theta1 = log x, D theta1 = 1/x
(define tL (rt-theta 1))
(define oneoverx (rat-make (list 1) (list 0 1)))
(must "D(log x) = 1/x" (rt-equal? 1 (rt-deriv 1 sL tL) (rt-lift1 0 oneoverx)))
(must "D(1/log x) = -(1/x)/(log x)^2   (RATIONAL in the monomial)"
      (rt-equal? 1 (rt-deriv 1 sL (rt-inv 1 tL)) (rt-neg 1 (rt-mul 1 (rt-lift1 0 oneoverx) (rt-mul 1 (rt-inv 1 tL) (rt-inv 1 tL))))))
(define s2 (list (list (quote exp) (rat-one)) (list (quote exp) (rt-theta 1))))   ; theta2 = exp(e^x)
(define t2 (rt-theta 2))
(must "D(exp(e^x)) = e^x exp(e^x)                      [depth 2]" (rt-equal? 2 (rt-deriv 2 s2 t2) (rt-mul 2 (rt-lift1 1 (rt-theta 1)) t2)))
(must "D(1/exp(e^x)) = -e^x/exp(e^x)   (negative power) [depth 2]" (rt-equal? 2 (rt-deriv 2 s2 (rt-inv 2 t2)) (rt-neg 2 (rt-mul 2 (rt-lift1 1 (rt-theta 1)) (rt-inv 2 t2)))))

; ===== (2) multi-residue Rothstein-Trager logarithmic part, certified =====
(newline)
(display "multi-residue logarithmic part (several logs at once), certified at depth:") (newline)
(define spec1 (list (quote exp) (rat-one)))
(define V1 (list (rat-neg (rat-one)) (rat-zero) (rat-one)))           ; theta1^2 - 1
(define P1 (list (rat-zero) (rat-scale 2 (rat-one))))                  ; 2 theta1
(define r1 (rt-integrate-logpart 1 s1 P1 V1))
(display "  INT 2e^x/(e^(2x)-1) dx -> ") (display (car r1)) (display ", ") (display (length (car (cdr (cdr r1))))) (display " logs") (newline)
(must "INT 2e^x/(e^(2x)-1) dx = log(e^x-1) - log(e^x+1)   (two residues) certified" (rt-integrate-logpart-decides? 1 s1 P1 V1))

(define VL (list (rat-neg (rat-one)) (rat-zero) (rat-one)))           ; (log x)^2 - 1
(define PL (list (rat-make (list 2) (list 0 1))))                      ; 2/x
(must "INT (2/x)/((log x)^2-1) dx = log(log x-1) - log(log x+1)  (NESTED-log arguments) certified" (rt-integrate-logpart-decides? 1 sL PL VL))

(define V2 (list (rt-neg 1 (rt-one 1)) (rt-zero 1) (rt-one 1)))       ; theta2^2 - 1 over level 1
(define P2 (list (rt-zero 1) (rt-mul 1 (rt-from-int 1 2) (rt-theta 1)))) ; 2 theta1 theta2
(must "INT 2e^x exp(e^x)/(exp(2e^x)-1) dx                 [DEPTH 2, two residues] certified" (rt-integrate-logpart-decides? 2 s2 P2 V2))

; ===== soundness: complex residues are NOT claimed elementary =====
(newline)
(display "soundness (no false positives):") (newline)
(define Virr (list (rat-one) (rat-zero) (rat-one)))                    ; theta1^2 + 1 (irreducible over Q)
(define Pirr (list (rat-zero) (rat-scale 2 (rat-one))))
(define rirr (rt-integrate-logpart 1 s1 Pirr Virr))
(display "  INT 2e^x/(e^(2x)+1) dx -> ") (display (car rirr)) (display "  (complex residues; correctly NOT elementary over Q)") (newline)
(must "complex-residue integral reported non-elementary, not a false RootSum" (not (equal? (car rirr) (quote elementary))))

(newline)
(display "rational tower + multi-residue RootSum: division at every level, several logs at once, nested-log arguments, certified, sound.") (newline)
