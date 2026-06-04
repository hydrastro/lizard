; -*- lisp -*-
; lib/cas/puiseux.lisp -- RUNG 4 (start) of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): PUISEUX
; EXPANSIONS of algebraic functions, the foundation for handling GENERAL algebraic functions (beyond sqrt:
; y^e = g(x) and ultimately arbitrary F(x,y) = 0), which is what lifts the integrator past the hyperelliptic
; restriction.
;
; A Puiseux series at the place over x = 0 is y(x) = sum_{k >= k0} c_k x^(k/e), a power series in a fractional
; power x^(1/e); e is the ramification index of the place.  By Newton-Puiseux every algebraic function has such
; an expansion (e branches over a ramified place).  This module computes it for the SUPERELLIPTIC case
; y^e = g(x) -- the n-th-root functions, the direct generalization of sqrt(p) and the case that matters first
; for algebraic integration.
;
; Method.  Write g(x) = x^v * gt(x) with gt(0) != 0 (v = ord_0 g).  Then
;     y = g^(1/e) = x^(v/e) * gt(0)^(1/e) * (1 + (gt/gt(0) - 1))^(1/e),
; and (1 + u)^(1/e) is the binomial series in u = gt/gt(0) - 1 (a power series in x with zero constant term).
; In the uniformizer t = x^(1/E) with E = e / gcd(v, e) (the true ramification index, the reduced denominator
; of v/e), every exponent k/e becomes an integer power of t, so the branch is the t-series
;     y = leadcoef * t^(v/g) * (series in t),     where the series is the binomial expansion re-expressed in t.
; We return (list 'puiseux E lead-exp coeffs), meaning  y = sum_i coeffs[i] * x^((lead-exp + i)/E)  ... i.e.
; coeffs are the successive t = x^(1/E) coefficients starting at t^lead-exp.  When gt(0)^(1/e) is irrational
; the leading constant is reported symbolically (kept exact when it is a perfect e-th power, else 'needs-radical).
;
; This is the genus-/ramification-aware local expansion; the integral-basis construction of the rest of Rung 4
; consumes these branch expansions at each singular place.  Builds on poly.lisp (x-polynomials) and series.lisp.

(import "cas/poly.lisp")
(import "cas/series.lisp")

(define (px-nth l k) (if (= k 0) (car l) (px-nth (cdr l) (- k 1))))
(define (px-idx l i) (if (< i (length l)) (px-nth l i) 0))

; ord_0 of a polynomial (lowest power of x with nonzero coeff); for the zero poly returns 'inf
(define (px-ord g) (if (null? g) (quote inf) (px-ord-go g 0)))
(define (px-ord-go g k) (cond ((null? g) (quote inf)) ((not (= (car g) 0)) k) (else (px-ord-go (cdr g) (+ k 1)))))

; drop the lowest v powers: g(x) = x^v * gt(x); return gt (coefficient list starting at the constant gt(0))
(define (px-shiftdown g v) (if (= v 0) g (px-shiftdown (cdr g) (- v 1))))

; binomial (1+u)^alpha coefficients c_0..c_M  (c_0=1, c_k = c_{k-1}(alpha-k+1)/k)
(define (px-binom alpha M) (px-binom-go alpha M 1 1 (list 1)))
(define (px-binom-go alpha M k cprev acc)
  (if (> k M) acc
      (let ((ck (/ (* cprev (- alpha (- k 1))) k)))
        (px-binom-go alpha M (+ k 1) ck (append acc (list ck))))))

; gcd of two nonnegative integers
(define (px-gcd a b) (if (= b 0) a (px-gcd b (remainder a b))))

; perfect e-th-root of a rational q, or 'needs-radical
(define (px-root q e)
  (if (= e 1) q
      (if (= e 2) (px-sqrt-q q)
          (let ((r (px-introot (if (< q 0) (- 0 q) q) e)))   ; only handle q>0 nth roots generally
            (if (equal? r (quote no)) (quote needs-radical) (if (< q 0) (quote needs-radical) r))))))
(define (px-sqrt-q q)
  (if (< q 0) (quote needs-radical)
      (let ((n (numerator q)) (d (denominator q)))
        (let ((rn (px-isqrt n)) (rd (px-isqrt d)))
          (if (if (equal? rn (quote no)) #t (equal? rd (quote no))) (quote needs-radical) (/ rn rd))))))
(define (px-isqrt n) (px-isqrt-go n 0))
(define (px-isqrt-go n k) (cond ((= (* k k) n) k) ((> (* k k) n) (quote no)) (else (px-isqrt-go n (+ k 1)))))
(define (px-introot n e) (px-introot-go n e 0))
(define (px-introot-go n e k) (let ((pk (px-ipow k e))) (cond ((= pk n) k) ((> pk n) (quote no)) (else (px-introot-go n e (+ k 1))))))
(define (px-ipow b e) (if (= e 0) 1 (* b (px-ipow b (- e 1)))))

; ----- the superelliptic Puiseux expansion of y for y^e = g(x) at x = 0, to M terms in the uniformizer -----
; returns (list 'puiseux E lead coeffs)  with  y = sum_{i} coeffs[i] x^((lead+i)/E),  E the ramification index,
;        or (list 'needs-radical ...) when the leading coefficient is not a rational e-th power.
(define (px-superelliptic g e M)
  (let ((v (px-ord g)))
    (if (equal? v (quote inf)) (list (quote puiseux) 1 0 (list 0))     ; g = 0 -> y = 0
        (let ((gt (px-shiftdown g v)))                                   ; gt(0) != 0
          (let ((g0 (car gt)))
            (let ((lead (px-root g0 e)))
              (if (equal? lead (quote needs-radical)) (list (quote needs-radical) (quote leading-coeff) g0)
                  (px-superelliptic-build g gt g0 e v lead M))))))))

(define (px-superelliptic-build g gt g0 e v lead M)
  (let ((d (px-gcd v e)))
    (let ((E (quotient e d)))                                           ; true ramification index
      ; u = gt/gt(0) - 1  as an x-power-series (constant term 0)
      (let ((u (px-series-of (px-scale-list (/ 1 g0) gt) M)))           ; gt/g0 as a series, then subtract 1
        (let ((u0 (px-sub-const u 1)))
          ; (1+u)^(1/e): compose binomial series with u
          (let ((bin (px-binom (/ 1 e) M)))
            (let ((s (px-compose-1plus bin u0 M)))                      ; series in x: (gt/g0)^(1/e)
              ; y = lead * x^(v/e) * s(x).  In t = x^(1/E): x = t^E, x^(v/e) = t^(v E/e) = t^(v/d).
              (let ((leadexp (quotient v d)))                            ; v/d = exponent of t for the lead
                (list (quote puiseux) E leadexp (px-scale-list lead (px-expand-in-t s E M)))))))))))

; helpers on coefficient lists treated as x-power-series (index = power of x)
(define (px-scale-list c l) (if (null? l) (quote ()) (cons (* c (car l)) (px-scale-list c (cdr l)))))
(define (px-series-of l M) (px-trunc l (+ M 1)))
(define (px-trunc l n) (if (= n 0) (quote ()) (if (null? l) (cons 0 (px-trunc (quote ()) (- n 1))) (cons (car l) (px-trunc (cdr l) (- n 1))))))
(define (px-sub-const l c) (cons (- (car l) c) (cdr l)))
; compose: given binomial coeffs b_k (for (1+u)^alpha = sum b_k u^k) and series u (const term 0), return series
(define (px-compose-1plus b u M) (px-comp-go b u M 0 (px-const-series 1 M) (px-zeros (+ M 1))))
; Horner-free: sum_k b_k u^k.  acc-power = u^k (series), result accumulates.
(define (px-comp-go b u M k upow result)
  (if (> k M) result
      (let ((term (px-smul (list (px-idx b k)) upow M)))   ; b_k * u^k  (b_k scalar)
        (px-comp-go b u M (+ k 1) (px-smul u upow M) (px-sadd result (px-scale-list (px-idx b k) upow))))))
(define (px-const-series c M) (cons c (px-zeros M)))
(define (px-zeros k) (if (= k 0) (quote ()) (cons 0 (px-zeros (- k 1)))))
(define (px-sadd a b) (if (null? a) b (if (null? b) a (cons (+ (car a) (car b)) (px-sadd (cdr a) (cdr b))))))
(define (px-smul a b M) (px-smul-go a b M 0))
(define (px-smul-go a b M k) (if (> k M) (quote ()) (cons (px-sconv a b k) (px-smul-go a b M (+ k 1)))))
(define (px-sconv a b k) (px-sconv-go a b k 0 0))
(define (px-sconv-go a b k i s) (if (> i k) s (px-sconv-go a b k (+ i 1) (+ s (* (px-idx a i) (px-idx b (- k i)))))))
; re-express an x-power-series s (s = sum s_i x^i) in t = x^(1/E): x^i = t^(iE). returns t-series to M terms.
(define (px-expand-in-t s E M) (px-expand-go s E M 0 (px-zeros (+ M 1))))
(define (px-expand-go s E M i acc)
  (if (> i M) acc
      (px-expand-go s E M (+ i 1) (px-set acc (* i E) (px-idx s i)))))
(define (px-set l idx val) (if (= idx 0) (cons val (if (null? l) (quote ()) (cdr l))) (cons (if (null? l) 0 (car l)) (px-set (if (null? l) (quote ()) (cdr l)) (- idx 1) val))))

; ----- verification: square/e-th-power the Puiseux series and compare to g, in the uniformizer -----
; for y^e = g: reconstruct y as t-series, raise to e, should equal g(t^E) as t-series.
(define (px-verify g e M)
  (let ((r (px-superelliptic g e M)))
    (if (equal? (car r) (quote puiseux))
        (let ((E (px-nth r 1)) (lead (px-nth r 2)) (coeffs (px-nth r 3)))
          ; y as t-series with leading exponent 'lead': shift coeffs up by 'lead'
          (let ((yt (px-shiftup coeffs lead)))
            (let ((ye (px-tpow yt e M)))
              (let ((gt (px-expand-in-t (px-trunc g (+ M 1)) E M)))
                (px-series-eq? ye gt (- M e))))))   ; compare up to safe order
        #f)))
(define (px-shiftup l k) (if (= k 0) l (cons 0 (px-shiftup l (- k 1)))))
(define (px-tpow l e M) (if (= e 0) (px-const-series 1 M) (px-smul l (px-tpow l (- e 1) M) M)))
(define (px-series-eq? a b upto) (px-eq-go a b 0 upto))
(define (px-eq-go a b k upto) (if (> k upto) #t (if (= (px-idx a k) (px-idx b k)) (px-eq-go a b (+ k 1) upto) #f)))
