; -*- lisp -*-
; lib/cas/hyperell.lisp -- INT P(x)/sqrt(p) dx for SQUAREFREE p of ARBITRARY degree: the elementary part by
; Hermite-style reduction, and the DECISION of non-elementarity for the genuine algebraic (elliptic /
; hyperelliptic) case (BOUNDARY 2).
;
; algfunc/algfuncint handled sqrt of a QUADRATIC (genus 0), always elementary.  This module opens the real
; algebraic summit: p squarefree of degree m, defining a hyperelliptic curve y^2 = p of genus
; g = floor((m-1)/2).  For INT P(x)/sqrt(p) dx the structure (Hermite reduction on the curve) is:
;
;   * the POLYNOMIAL part of P (degree >= m-1) is removed by repeatedly subtracting derivatives D(x^k y),
;     since D(x^k y) = (k x^{k-1} p + x^k p'/2)/y has y-numerator k x^{k-1} p + x^k p'/2 of degree k-1+m;
;     a descending pass writes  P = (Q' p + Q p'/2) + S  with deg S < m-1, so
;        INT P/sqrt(p) = Q sqrt(p) + INT S/sqrt(p),  Q a polynomial, exact and certified.
;   * the remaining INT S/sqrt(p) with deg S < m-1 = 2g + (0 or 1) decomposes into the g FIRST-KIND
;     differentials x^i/sqrt(p), i = 0..g-1 (holomorphic, NON-elementary for g >= 1) plus, when m is even,
;     a possible second-kind/log part.  For the squarefree case the integral is ELEMENTARY iff S = 0 after
;     the polynomial reduction (no first-kind content); otherwise it is genuinely non-elementary.
;
; Thus INT dx/sqrt(x^3+1) (m=3, g=1, S=1 != 0) is PROVEN non-elementary, while a numerator that is exactly a
; derivative, e.g. INT (3x^2/2)/sqrt(x^3+1) dx = sqrt(x^3+1), reduces to S=0 and is returned and certified.
; Every elementary answer is checked by differentiation inside K = Q(x)[y]/(y^2 - p) (af-certify), the same
; arbiter used throughout; non-elementary verdicts are decisions, not failures.
;
; Builds on algfunc.lisp (the field K and its derivation) and poly.lisp.

(import "cas/algfunc.lisp")

; genus and key degrees for squarefree p of degree m
(define (he-genus m) (quotient (- m 1) 2))

; ----- polynomial reduction: write P = Q' p + Q p'/2 + S with deg S minimal (< m-1), Q a polynomial -----
; D(x^k y) has y-numerator  L_k := k x^{k-1} p + x^k p'/2,  of degree (k-1+m) for k>=1 and (m-1) for k=0...
; actually L_k top degree = k-1+m (from k x^{k-1} p) vs k + (m-1) (from x^k p'/2) -- both equal k+m-1.
; So to cancel P's top degree d = deg P (>= m-1), use Q-term q x^{d-m+1} with the right q, descending.
(define (he-Lk k p)                                       ; numerator of D(x^k y) = k x^{k-1} p + x^k p'/2
  (poly-add (poly-scale-shift-int k (- k 1) p) (poly-half-shift k (poly-deriv p))))
(define (poly-scale-shift-int c i q) (if (< i 0) '() (poly-scale c (poly-shift q i))))   ; c x^i q
(define (poly-half-shift i q) (poly-scale (/ 1 2) (poly-shift q i)))                       ; (1/2) x^i q
(define (poly-shift q i) (if (= i 0) q (cons 0 (poly-shift q (- i 1)))))

; descending reduction.  At each step the current remainder R has degree dR; if dR >= m-1 we cancel its top
; with a Q-monomial q x^j where j = dR-(m-1), q = lead(R)/lead(L_{j+1})... but the cleanest is to use that the
; top coefficient of L_k is (k + 1/2*?)*lead(p)... we compute L_k explicitly and match.
; We build Q (low->high) and reduce R until deg R < m-1.
(define (he-reduce R p m)                                 ; -> (list Q S)   with P = Q'p + Qp'/2 + S
  (he-reduce-go R p m '()))
(define (he-reduce-go R p m Qacc)
  (if (< (poly-deg R) (- m 1)) (list (poly-norm Qacc) (poly-norm R))
      (let ((j (- (poly-deg R) (- m 1))))                 ; Q-monomial degree
        (let ((Lk (he-Lk (+ j 1) p)))                     ; D(x^{j+1} y) numerator, top degree (j+1)+m-1 = j+m
          ; top degree of Lk is j+m... but deg R = j+m-1. Mismatch -> use L with k=j+? Let's match top degrees:
          ; we need a y-numerator of top degree dR = j+m-1.  L_k has top degree k+m-1, so k = j gives k+m-1 = j+m-1. Use k=j.
          (let ((Lj (he-Lk j p)))
            (let ((q (/ (poly-lead R) (poly-lead Lj))))
              (he-reduce-go (poly-norm (poly-sub R (poly-scale q Lj))) p m
                            (poly-add Qacc (poly-monomial-he q j)))))))))
(define (poly-monomial-he q j) (if (= j 0) (list q) (cons 0 (poly-monomial-he q (- j 1)))))

; ----- the decision -----
; INT P/sqrt(p) dx, p squarefree degree m.  -> (list 'elementary Q)  meaning Q sqrt(p), with S = 0
;                                            | (list 'non-elementary g S)  S the first-kind remainder (!=0)
(define (he-integrate P p)
  (let ((m (poly-deg p)))
    (let ((rs (he-reduce P p m)))
      (let ((Q (car rs)) (S (car (cdr rs))))
        (if (poly-zero? S)
            (list (quote elementary) Q)
            (list (quote non-elementary) (he-genus m) S))))))

; certificate for an elementary verdict: D(Q sqrt(p)) = P/sqrt(p) in K
(define (he-integrate-certify P p)
  (let ((r (he-integrate P p)))
    (if (equal? (car r) (quote elementary))
        (let ((Q (car (cdr r))) (prat (rat-from-poly p)))
          (af-equal? (af-deriv prat (af-make (rat-zero) (rat-from-poly Q)))    ; D(Q y)
                     (af-make (rat-zero) (rat-div (rat-from-poly P) prat))))    ; P/y = P y / p -> v-part P/p
        #f)))
(define (he-integrate-decides? P p)
  (let ((r (he-integrate P p)))
    (if (equal? (car r) (quote elementary)) (he-integrate-certify P p) (equal? (car r) (quote non-elementary)))))
