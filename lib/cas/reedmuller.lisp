; -*- lisp -*-
; lib/cas/reedmuller.lisp -- first-order Reed-Muller codes RM(1, m).
;
; The codewords of RM(1, m) are the truth tables of affine Boolean functions
;   f(x_1,...,x_m) = a_0 XOR a_1 x_1 XOR ... XOR a_m x_m,
; tabulated over all 2^m inputs.  This is a [2^m, m+1, 2^{m-1}] code: 2^{m+1} codewords,
; minimum distance 2^{m-1}, correcting up to t = 2^{m-2} - 1 errors.
;
; Decoding is maximum-likelihood by the fast Walsh-Hadamard transform.  Map the received
; bits to signs (b -> 1 - 2b) and transform; each affine function corresponds to one
; transform coefficient, so the coefficient of largest magnitude names the nearest codeword:
; its index gives the linear coefficients (a_1..a_m) and its sign gives a_0.  The whole thing
; is self-contained over GF(2) and certified by the round trip -- corrupt a codeword in up to
; t positions and decoding returns the original message.

; ---------- bit utilities over GF(2) ----------
(define (xor a b) (if (= a b) 0 1))
(define (bit x k) (remainder (quotient x (expt 2 k)) 2))
(define (rm-length m) (expt 2 m))
(define (rm-dim m) (+ m 1))
(define (rm-distance m) (expt 2 (- m 1)))
(define (rm-t m) (- (expt 2 (- m 2)) 1))

; ---------- list helpers ----------
(define (take-n l n) (if (or (= n 0) (null? l)) '() (cons (car l) (take-n (cdr l) (- n 1)))))
(define (drop-n l n) (if (or (= n 0) (null? l)) l (drop-n (cdr l) (- n 1))))
(define (map2 f a b) (if (or (null? a) (null? b)) '() (cons (f (car a) (car b)) (map2 f (cdr a) (cdr b)))))
(define (nth0 l i) (if (= i 0) (car l) (nth0 (cdr l) (- i 1))))
(define (set-nth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (set-nth (cdr l) (- i 1) v))))

; ---------- encode: message (a_0 a_1 ... a_m) -> truth table over 0..2^m-1 ----------
(define (lin-part coeffs x k m)               ; XOR_{k=1..m} a_k * bit_{k-1}(x)
  (if (> k m) 0 (xor (if (= (bit x (- k 1)) 1) (nth0 coeffs k) 0) (lin-part coeffs x (+ k 1) m))))
(define (rm-encode coeffs m) (rm-enc coeffs m 0 (rm-length m)))
(define (rm-enc coeffs m x n)
  (if (>= x n) '() (cons (xor (car coeffs) (lin-part coeffs x 1 m)) (rm-enc coeffs m (+ x 1) n))))

; ---------- corrupting ----------
(define (rm-corrupt cw positions) (if (null? positions) cw (rm-corrupt (set-nth cw (car positions) (xor (nth0 cw (car positions)) 1)) (cdr positions))))

; ---------- fast Walsh-Hadamard transform (natural order) ----------
(define (vadd a b) (map2 + a b))
(define (vsub a b) (map2 - a b))
(define (wht v)
  (if (= (length v) 1) v
      (let ((h (quotient (length v) 2)))
        (let ((a (wht (take-n v h))) (b (wht (drop-n v h))))
          (append (vadd a b) (vsub a b))))))

; ---------- decode ----------
(define (signs r) (map (lambda (b) (- 1 (* 2 b))) r))
(define (argmax-abs w) (am-go w 0 0 -1))                 ; index of largest |w_i|
(define (am-go w i best bestval) (cond ((null? w) best) ((> (abs (car w)) bestval) (am-go (cdr w) (+ i 1) i (abs (car w)))) (else (am-go (cdr w) (+ i 1) best bestval))))
(define (coeff-at w j) (nth0 w j))
(define (bits-of j m) (bo-go j 1 m))                     ; (a_1 ... a_m) = low..high bits of j
(define (bo-go j k m) (if (> k m) '() (cons (bit j (- k 1)) (bo-go j (+ k 1) m))))
(define (rm-decode r m)
  (let ((w (wht (signs r))))
    (let ((j (argmax-abs w)))
      (cons (if (>= (coeff-at w j) 0) 0 1) (bits-of j m)))))

; ---------- certificates ----------
(define (rm-clean-ok? coeffs m) (equal? (rm-decode (rm-encode coeffs m) m) coeffs))
(define (rm-roundtrip-ok? coeffs m positions) (equal? (rm-decode (rm-corrupt (rm-encode coeffs m) positions) m) coeffs))

; ---------- display ----------
(define (rm-info m) (string-append "RM(1," (number->string m) ") = [" (number->string (rm-length m)) "," (number->string (rm-dim m)) "," (number->string (rm-distance m)) "] code, corrects " (number->string (rm-t m))))
