; -*- lisp -*-
; lib/cas/hermite.lisp -- Hermite reduction: the rational part of integration.
;
; For a proper rational function A/D, Hermite reduction writes
;   INT A/D dx = (rational part) + INT (numerator)/(radical of D) dx,
; where the radical of D (its squarefree part) carries only simple roots, so the remaining
; integral is a pure logarithmic part (handed to Rothstein-Trager).  The method needs no
; factorization into irreducibles: it squarefree-factorizes D = prod V_k^k (Yun), splits A/D
; into the pieces A_k/V_k^k by the polynomial CRT, and on each piece integrates by parts using
; the Bezout relation S V + T V' = 1 (V squarefree, so gcd(V,V')=1) to peel one power at a
; time, accumulating the rational part.  Everything is exact rational arithmetic and the result
; is certified by differentiation: d/dx(rational part) + residual/radical = A/D exactly.
; Builds on resultant.lisp (which brings poly.lisp).

(import "cas/resultant.lisp")

; ---------- extended Euclid for polynomials (own, prefixed) ----------
(define (hm-eea r0 r1 s0 s1 t0 t1)
  (if (poly-zero? r1) (list r0 s0 t0)
      (let ((q (poly-div r0 r1)))
        (hm-eea r1 (poly-sub r0 (poly-mul q r1)) s1 (poly-sub s0 (poly-mul q s1)) t1 (poly-sub t0 (poly-mul q t1))))))
(define (hm-bezout1 a b)                       ; coprime a,b -> (S T) with S a + T b = 1
  (let ((g (hm-eea a b (list 1) '() '() (list 1))))
    (let ((c (poly-coeff (car g) 0)))
      (list (poly-scale (/ 1 c) (car (cdr g))) (poly-scale (/ 1 c) (car (cdr (cdr g))))))))
(define (hm-pow p k) (if (= k 0) (list 1) (poly-mul p (hm-pow p (- k 1)))))

; ---------- rational-function bookkeeping (num . den), unreduced ----------
(define (hm-radd n1 d1 n2 d2) (cons (poly-add (poly-mul n1 d2) (poly-mul n2 d1)) (poly-mul d1 d2)))

; ---------- single squarefree factor: reduce INT a/V^m -> (ratnum ratden residnum) ----------
;   residnum/V is the leftover simple-denominator integrand
(define (hm-one a V m) (hm-step a V m '() (list 1)))
(define (hm-step a V m rn rd)
  (if (= m 1) (list rn rd a)
      (let ((st (hm-bezout1 V (poly-deriv V))))
        (let ((bS (car st)) (bT (car (cdr st))))
          (let ((aT (poly-mul a bT)))
            (let ((nr (hm-radd rn rd (poly-scale (/ -1 (- m 1)) aT) (hm-pow V (- m 1)))))
              (hm-step (poly-add (poly-mul a bS) (poly-scale (/ 1 (- m 1)) (poly-deriv aT)))
                       V (- m 1) (car nr) (cdr nr))))))))

; ---------- CRT split of A/D into pieces A_k / V_k^k ----------
(define (hm-prod-others facts skip)            ; product of V_j^j for j != skip (by index)
  (hp facts skip 0 (list 1)))
(define (hp facts skip i acc)
  (if (null? facts) acc
      (hp (cdr facts) skip (+ i 1)
          (if (= i skip) acc (poly-mul acc (hm-pow (car (cdr (car facts))) (car (car facts))))))))
(define (hm-piece A facts i)                    ; numerator A_i for the i-th factor (k V)
  (let ((k (car (list-ref-hm facts i))) (V (car (cdr (list-ref-hm facts i)))))
    (let ((Vk (hm-pow V k)) (M (hm-prod-others facts i)))
      (let ((S (car (hm-bezout1 M Vk))))
        (poly-rem (poly-mul A S) Vk)))))
(define (list-ref-hm l i) (if (= i 0) (car l) (list-ref-hm (cdr l) (- i 1))))
(define (hm-len l) (if (null? l) 0 (+ 1 (hm-len (cdr l)))))

; ---------- full Hermite reduction ----------
; returns (ratnum ratden resnum resden) : INT A/D = ratnum/ratden + INT resnum/resden, resden squarefree
(define (hermite A D)
  (let ((facts (square-free (poly-monic D))))
    (let ((lc (poly-lead D)))
      (hermite-go (poly-scale (/ 1 lc) A) facts 0 (hm-len facts) '() (list 1) '() (list 1)))))
(define (hermite-go A facts i n rn rd sn sd)
  (if (>= i n) (list rn rd sn sd)
      (let ((k (car (list-ref-hm facts i))) (V (car (cdr (list-ref-hm facts i)))))
        (let ((Ai (hm-piece A facts i)))
          (let ((red (hm-one Ai V k)))           ; (ratnum ratden residnum)  residnum/V
            (let ((nr (hm-radd rn rd (car red) (car (cdr red))))
                  (ns (hm-radd sn sd (car (cdr (cdr red))) V)))
              (hermite-go A facts (+ i 1) n (car nr) (cdr nr) (car ns) (cdr ns))))))))

; ---------- certificate: d/dx(ratnum/ratden) + resnum/resden = A/D ----------
(define (hm-ratderiv n d) (cons (poly-sub (poly-mul (poly-deriv n) d) (poly-mul n (poly-deriv d))) (poly-mul d d)))
(define (hermite-verify A D)
  (let ((h (hermite A D)))
    (let ((rn (car h)) (rd (car (cdr h))) (sn (car (cdr (cdr h)))) (sd (car (cdr (cdr (cdr h))))))
      (let ((dr (hm-ratderiv rn rd)))             ; (num . den) of d/dx(rat part)
        (let ((lhs (hm-radd (car dr) (cdr dr) sn sd)))   ; total integrand reconstructed
          ; compare lhs (num . den) with A/D :  num*D == A*den
          (poly-zero? (poly-sub (poly-mul (car lhs) D) (poly-mul A (cdr lhs)))))))))
(define (hermite-radical D) (hr (square-free (poly-monic D)) (list 1)))
(define (hr facts acc) (if (null? facts) acc (hr (cdr facts) (poly-mul acc (car (cdr (car facts)))))))
