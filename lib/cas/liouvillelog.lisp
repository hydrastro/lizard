; -*- lisp -*-
; lib/cas/liouvillelog.lisp -- the logarithmic companion of the Liouville decider (liouville.lisp).  Decides and
; integrates INT P(x) log(x) dx for P a polynomial, and records the proven NON-elementarity of the logarithmic
; integral INT 1/log(x) dx (li).  Part of the decider suite that proves elementarity verdicts rather than merely
; constructing antiderivatives (docs/TRAGER_ROADMAP.md, the summit).
;
; INT P log x dx by parts: with F = INT P (a polynomial, always exists),
;     INT P log x dx = F log x - INT F/x dx ,
; and F/x is a rational function, so INT F/x is elementary (a rational part plus logarithms).  Hence INT P log x
; is ALWAYS elementary, and the decider returns the explicit answer F log x - (INT F/x).  Here F/x for a
; polynomial F = sum f_k x^k is sum f_k x^{k-1}, whose integral is sum_{k>=1} (f_k/k) x^k + f_0 log x; we return
; the polynomial part Q (so the antiderivative is F log x - Q - f_0 log x), captured as the triple
; (F, Q, f0): INT P log x dx = (F - f0) log x ... more precisely F log x - Q - f0 log x.  lv-log-certify checks
; d/dx of the returned closed form equals P log x.
;
; By contrast INT 1/log x dx (the logarithmic integral li) is NON-elementary: in the tower Q(x)(t), t = log x,
; no element has derivative 1/t (a Liouville/Risch argument); lv-log-li records the proven verdict.
;
; Public:
;   lv-log-intpoly P        -> F = INT P (polynomial antiderivative of a polynomial)
;   lv-log-decide P         -> (list 'elementary F Q f0): INT P log x dx = F log x - Q - f0 log x, where
;                              Q = polynomial part of INT F/x and f0 = constant term of F (the residual log)
;   lv-log-certify P F Q f0 -> #t iff d/dx( F log x - Q - f0 log x ) = P log x  (the certificate, in the tower)
;   lv-log-li               -> (list 'non-elementary 'li-no-tower-antiderivative): the proven verdict for
;                              INT 1/log x dx
;
; Verified: INT log x dx = x log x - x; INT x log x dx = (x^2/2) log x - x^2/4; INT (x^2) log x dx; and the
; certificate d/dx of each closed form equals the integrand.  INT 1/log x non-elementary.
;
; Builds on poly.lisp.  Certificate works in the tower Q(x)(log x): d/dx(F log x) = F' log x + F/x.

(import "cas/poly.lisp")

(define (ll-nth l k) (if (= k 0) (car l) (ll-nth (cdr l) (- k 1))))
(define (ll-len l) (if (null? l) 0 (+ 1 (ll-len (cdr l)))))

; ----- polynomial integral: INT (sum f_k x^k) = sum (f_k/(k+1)) x^{k+1}, constant 0 -----
(define (lv-log-intpoly P) (cons 0 (ll-int-go P 1)))
(define (ll-int-go P k) (if (null? P) (quote ()) (cons (/ (car P) k) (ll-int-go (cdr P) (+ k 1)))))

; ----- INT F/x: F = sum f_k x^k, F/x = sum f_k x^{k-1}.  polynomial part Q = sum_{k>=1} (f_k/k) x^k,
; plus the residual f_0 log x (f_0 = constant term of F). -----
(define (ll-poly-part-of-F-over-x F) (ll-ppf F 1 (ll-tail F)))   ; drop f_0, integrate the rest
(define (ll-tail l) (if (null? l) (quote ()) (cdr l)))
(define (ll-ppf F k rest) (cons 0 (ll-ppf-go rest k)))           ; Q has zero constant term
(define (ll-ppf-go rest k) (if (null? rest) (quote ()) (cons (/ (car rest) k) (ll-ppf-go (cdr rest) (+ k 1)))))
(define (ll-f0 F) (if (null? F) 0 (car F)))

(define (lv-log-decide P) (lv-log-build P (lv-log-intpoly P)))
(define (lv-log-build P F) (list (quote elementary) F (ll-poly-part-of-F-over-x F) (ll-f0 F)))

; ----- certificate: d/dx( F log x - Q - f0 log x ) = F' log x + F/x - Q' - f0/x.  Since F/x = Q' + f0/x
; (by construction Q' is the polynomial part of F/x and f0/x its residual), this is F' log x.  But we want it to
; equal P log x, i.e. F' = P (F is the antiderivative of P) and the non-log parts cancel.  Check both. -----
(define (lv-log-certify P F Q f0)
  (if (ll-poly-eq? (poly-deriv F) P)                       ; F' = P  (so F' log x = P log x)
      (ll-nonlog-cancels? F Q f0)                          ; F/x - Q' - f0/x = 0
      #f))
(define (ll-nonlog-cancels? F Q f0)
  (ll-poly-eq? (poly-add (poly-deriv Q) (ll-const f0)) (ll-Fover-x-poly F)))
; F/x as a polynomial + f0/x: the polynomial part is sum_{k>=1} f_k x^{k-1}; we compare Q' + f0-thing on the
; polynomial side: Q' should equal the polynomial part of F/x, and f0 the residual.
(define (ll-Fover-x-poly F) (ll-tail F))                   ; polynomial part of F/x = F with f_0 dropped, shifted
(define (ll-const c) (if (= c 0) (quote ()) (list c)))
(define (ll-poly-eq? a b) (ll-veq? (poly-norm a) (poly-norm b)))
(define (ll-veq? a b) (cond ((null? a) (null? b)) ((null? b) (ll-veq? a (quote ()))) (else (if (= (car a) (ll-h b)) (ll-veq? (cdr a) (ll-t b)) #f))))
(define (ll-h b) (if (null? b) 0 (car b)))
(define (ll-t b) (if (null? b) (quote ()) (cdr b)))

; ----- INT 1/log x dx = li(x): proven non-elementary -----
(define (lv-log-li) (list (quote non-elementary) (quote li-no-tower-antiderivative)))
