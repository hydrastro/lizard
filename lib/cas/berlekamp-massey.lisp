; -*- lisp -*-
; lib/cas/berlekamp-massey.lisp -- shortest linear recurrence of a sequence over Q.
;
; Berlekamp-Massey finds, from the terms of a sequence s_0, s_1, ..., the SHORTEST linear
; recurrence  s_n = d_1 s_{n-1} + ... + d_L s_{n-L}  that generates it.  It maintains a
; connection polynomial C(x) = 1 + C_1 x + ... + C_L x^L; at each step the discrepancy
; d = sum_{i=0}^{L} C_i s_{n-i} is formed, and when it is nonzero the polynomial is
; corrected by a shifted multiple of the previous one, growing the register only when
; forced (2L <= n).  The recurrence coefficients are then d_i = -C_i.
;
; This is the exact dual of the linear-recurrence solver: that module SOLVES a known
; recurrence in closed form, while this one DISCOVERS the recurrence from raw terms.
; Composed, Berlekamp-Massey -> linrec turns a list of numbers into a closed form.  The
; discovered recurrence is certified by replaying it against every given term, so a
; recurrence that does not reproduce the data is never returned.  Self-contained over Q.

(define (pnth l i) (if (= i 0) (car l) (pnth (cdr l) (- i 1))))
(define (prange a b) (if (> a b) '() (cons a (prange (+ a 1) b))))
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (vget v i) (if (< i (length v)) (pnth v i) 0))
(define (vscale c v) (map (lambda (x) (* c x)) v))
(define (vshift m v) (append (zeros m) v))
(define (vlen2 a b) (if (> (length a) (length b)) (length a) (length b)))
(define (vadd a b) (va a b 0 (vlen2 a b)))
(define (va a b i n) (if (>= i n) '() (cons (+ (vget a i) (vget b i)) (va a b (+ i 1) n))))
(define (vsub a b) (vadd a (vscale -1 b)))

; discrepancy at step n: sum_{i=0}^{min(L,n)} C_i s_{n-i}
(define (disc C s n) (disc-go C s n 0 0))
(define (disc-go C s n i acc) (if (or (> i n) (>= i (length C))) acc (disc-go C s n (+ i 1) (+ acc (* (pnth C i) (pnth s (- n i)))))))

; ---------- main loop ----------
(define (bm s) (bm-loop s (list 1) (list 1) 0 1 1 0 (length s)))
(define (bm-loop s C B L m b n N)
  (if (>= n N) (bm-result C L)
    (let ((d (disc C s n)))
      (cond ((= d 0) (bm-loop s C B L (+ m 1) b (+ n 1) N))
            ((<= (* 2 L) n) (bm-loop s (vsub C (vscale (/ d b) (vshift m B))) C (+ (- n L) 1) 1 d (+ n 1) N))
            (else (bm-loop s (vsub C (vscale (/ d b) (vshift m B))) B L (+ m 1) b (+ n 1) N))))))
(define (bm-result C L) (map (lambda (i) (- 0 (vget C i))) (prange 1 L)))

; recurrence coefficients (d_1..d_L) and its order L
(define (bm-recurrence s) (bm s))
(define (bm-order s) (length (bm s)))

; ---------- certificate: replay the recurrence over the data ----------
(define (rdot d s n) (rd d s n 1 0))
(define (rd d s n i acc) (if (> i (length d)) acc (rd d s n (+ i 1) (+ acc (* (pnth d (- i 1)) (pnth s (- n i)))))))
(define (bm-rep s d n N) (cond ((>= n N) #t) ((= (pnth s n) (rdot d s n)) (bm-rep s d (+ n 1) N)) (else #f)))
(define (bm-ok? s) (let ((d (bm s))) (bm-rep s d (length d) (length s))))

; ---------- display ----------
(define (qn x) (if (integer? x) (number->string x) (string-append (number->string (numerator x)) "/" (number->string (denominator x)))))
(define (terms->string d i) (cond ((null? d) "") ((null? (cdr d)) (string-append (qn (car d)) "*a(n-" (number->string i) ")")) (else (string-append (qn (car d)) "*a(n-" (number->string i) ") + " (terms->string (cdr d) (+ i 1))))))
(define (bm->string s) (let ((d (bm s))) (if (null? d) "a(n) = 0" (string-append "a(n) = " (terms->string d 1)))))
