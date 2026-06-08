; -*- lisp -*-
; lib/cas/discretelog.lisp -- primitive roots and the discrete logarithm mod a prime.
;
; A primitive root g modulo a prime p generates the whole multiplicative group, i.e. its
; order is p-1.  This is tested without computing the order directly: g is a primitive
; root iff g^(p-1) = 1 and g^((p-1)/q) != 1 (mod p) for every prime q dividing p-1.  The
; smallest such g is returned.
;
; The discrete logarithm -- the x with g^x = h (mod p) -- is found by Shanks' baby-step
; giant-step method in O(sqrt p): tabulate g^0..g^{m-1}, then take giant strides of g^{-m}
; from h until a baby step matches, giving x = i*m + j.
;
; Both are certified: the discrete log is checked by raising g to the recovered exponent
; (g^x must equal h mod p), and the primitive root by confirming its order is exactly p-1
; through the independent order-mod routine.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (isqrt n) (if (< n 2) n (isqrt-bs n 1 n)))
(define (isqrt-bs n lo hi)
  (if (>= lo hi) (if (<= (* hi hi) n) hi (- hi 1))
    (let ((mid (quotient (+ lo hi 1) 2)))
      (if (<= (* mid mid) n) (isqrt-bs n mid hi) (isqrt-bs n lo (- mid 1))))))
(define (isqrt-ceil n) (let ((r (isqrt n))) (if (= (* r r) n) r (+ r 1))))

; ---------- primitive roots ----------
(define (distinct-primes n) (map car (factor-int n)))
(define (pr-check g p qs) (cond ((null? qs) #t) ((= (mod-exp g (quotient (- p 1) (car qs)) p) 1) #f) (else (pr-check g p (cdr qs)))))
(define (is-primitive-root? g p) (and (= (mod-exp g (- p 1) p) 1) (pr-check g p (distinct-primes (- p 1)))))
(define (pr-find p g) (if (is-primitive-root? g p) g (pr-find p (+ g 1))))
(define (primitive-root p) (pr-find p 2))

; ---------- discrete log: baby-step giant-step ----------
(define (alook v al) (cond ((null? al) 'none) ((= (car (car al)) v) (cdr (car al))) (else (alook v (cdr al)))))
(define (baby-steps g p m j cur acc) (if (>= j m) acc (baby-steps g p m (+ j 1) (imod (* cur g) p) (cons (cons cur j) acc))))
(define (giant g h p m baby ginvm i gamma)
  (if (>= i m) 'none
    (let ((hit (alook gamma baby)))
      (if (not (equal? hit 'none)) (+ (* i m) hit)
        (giant g h p m baby ginvm (+ i 1) (imod (* gamma ginvm) p))))))
(define (discrete-log g h p)
  (let ((m (isqrt-ceil (- p 1))))
    (giant g (imod h p) p m (baby-steps g p m 0 1 '()) (mod-exp (mod-inverse g p) m p) 0 (imod h p))))

; ---------- certificates ----------
(define (discrete-log-ok? g h p) (let ((x (discrete-log g h p))) (if (equal? x 'none) #t (= (mod-exp g x p) (imod h p)))))
(define (primitive-root-ok? p) (let ((g (primitive-root p))) (= (order-mod g p) (- p 1))))

; ---------- display ----------
(define (dlog->string g h p) (let ((x (discrete-log g h p))) (if (equal? x 'none) "none" (number->string x))))
