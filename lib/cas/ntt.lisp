; -*- lisp -*-
; lib/cas/ntt.lisp -- the Number-Theoretic Transform (NTT) over F_p.
;
; Over a prime field with a primitive n-th root of unity w (n a power of two dividing
; p - 1), the NTT is the finite-field analogue of the discrete Fourier transform:
; X_k = sum_j a_j w^{jk}.  It is computed here by the recursive radix-2 Cooley-Tukey
; splitting into even- and odd-indexed subtransforms, so in O(n log n) operations, and the
; inverse uses w^{-1} with a final scaling by n^{-1}.  Because the transform turns
; convolution into pointwise multiplication, it gives fast cyclic convolution and fast
; polynomial multiplication.
;
; Three facts certify it: the inverse transform recovers the input exactly; the inverse of
; the pointwise product of two transforms equals the direct cyclic convolution; and
; multiplying two polynomials through the NTT (with zero padding) matches the schoolbook
; product coefficient for coefficient.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

; ---------- a primitive n-th root of unity mod p ----------
(define (nth-root p n) (nr p n 2))
(define (nr p n g)
  (let ((c (mod-exp g (quotient (- p 1) n) p)))
    (if (and (= (mod-exp c n p) 1) (not (= (mod-exp c (quotient n 2) p) 1))) c (nr p n (+ g 1)))))

; ---------- the transform ----------
(define (map2 f a b) (if (or (null? a) (null? b)) '() (cons (f (car a) (car b)) (map2 f (cdr a) (cdr b)))))
(define (evens l) (if (null? l) '() (cons (car l) (evens (if (null? (cdr l)) '() (cdr (cdr l)))))))
(define (odds l) (if (or (null? l) (null? (cdr l))) '() (cons (car (cdr l)) (odds (cdr (cdr l))))))
(define (tw O wk w p) (if (null? O) '() (cons (imod (* wk (car O)) p) (tw (cdr O) (imod (* wk w) p) w p))))
(define (vadd a b p) (map2 (lambda (x y) (imod (+ x y) p)) a b))
(define (vsub a b p) (map2 (lambda (x y) (imod (- x y) p)) a b))
(define (ntt-step E O w p) (let ((T (tw O 1 w p))) (append (vadd E T p) (vsub E T p))))
(define (ntt a w p)
  (if (or (null? a) (null? (cdr a))) a
      (ntt-step (ntt (evens a) (imod (* w w) p) p) (ntt (odds a) (imod (* w w) p) p) w p)))
(define (intt A w p)
  (let ((ninv (mod-inverse (length A) p)))
    (map (lambda (x) (imod (* x ninv) p)) (ntt A (mod-inverse w p) p))))

; ---------- convolution and polynomial multiplication ----------
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (pad a n) (if (>= (length a) n) a (append a (zeros (- n (length a))))))
(define (pointwise A B p) (map2 (lambda (x y) (imod (* x y) p)) A B))
(define (ntt-cyclic a b w p n) (intt (pointwise (ntt (pad a n) w p) (ntt (pad b n) w p) p) w p))
(define (dropz a) (cond ((null? a) '()) ((= (car (rev a)) 0) (dropz (rev (cdr (rev a)))) ) (else a)))
(define (rev a) (reverse a))
(define (ntt-polymul a b w p n) (trimz (ntt-cyclic a b w p n)))
(define (trimz a) (reverse (dz (reverse a))))
(define (dz a) (cond ((null? a) '()) ((= (car a) 0) (dz (cdr a))) (else a)))

; ---------- reference: direct cyclic convolution and schoolbook product ----------
(define (nthc l i) (if (= i 0) (car l) (nthc (cdr l) (- i 1))))
(define (cyc a b p n) (cyc-go a b p n 0))
(define (cyc-go a b p n k) (if (>= k n) '() (cons (cyc-k a b p n k 0 0) (cyc-go a b p n (+ k 1)))))
(define (cyc-k a b p n k i acc) (if (>= i n) (imod acc p) (cyc-k a b p n k (+ i 1) (+ acc (* (nthc a i) (nthc b (imod (- k i) n)))))))
(define (school a b p) (trimz (cyc (pad a (+ (length a) (length b))) (pad b (+ (length a) (length b))) p (+ (length a) (length b)))))

; ---------- certificates ----------
(define (ntt-roundtrip-ok? a p n) (let ((w (nth-root p n))) (equal? (intt (ntt (pad a n) w p) w p) (pad a n))))
(define (ntt-convolution-ok? a b p n) (let ((w (nth-root p n))) (equal? (ntt-cyclic a b w p n) (cyc (pad a n) (pad b n) p n))))
(define (ntt-polymul-ok? a b p n) (let ((w (nth-root p n))) (equal? (ntt-polymul a b w p n) (school a b p))))

; ---------- display ----------
(define (vec->string v) (if (null? v) "()" (string-append "(" (vstr v) ")")))
(define (vstr v) (if (null? (cdr v)) (number->string (car v)) (string-append (number->string (car v)) " " (vstr (cdr v)))))
