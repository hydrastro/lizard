; -*- lisp -*-
; lib/rational.lisp — Exact rational arithmetic (fractions).
;
; A rational is represented as (rat numerator denominator), always
; stored in lowest terms with a positive denominator.

; ---- gcd helper (Euclid) ----
(define (rat-gcd a b)
  (if (= b 0) (rat-abs a) (rat-gcd b (remainder a b))))

(define (rat-abs n) (if (< n 0) (- 0 n) n))

; ---- construction (auto-normalizes) ----
(define (make-rat num den)
  (if (= den 0)
      (begin (display "  rat-error: zero denominator") (newline)
             (list 'rat 0 1))
      (let ((sign (if (< den 0) -1 1)))
        (let ((n (* sign num))
              (d (* sign den)))
          (let ((g (rat-gcd (rat-abs n) d)))
            (if (= g 0)
                (list 'rat 0 1)
                (list 'rat (quotient n g) (quotient d g))))))))

; ---- accessors ----
(define (rat-num r) (car (cdr r)))
(define (rat-den r) (car (cdr (cdr r))))
(define (rat? x) (and (pair? x) (equal? (car x) 'rat)))

; Coerce an integer to a rational.
(define (->rat x)
  (if (rat? x) x (make-rat x 1)))

; ---- arithmetic ----
; a/b + c/d = (ad + bc)/(bd)
(define (rat+ x y)
  (let ((a (->rat x)) (b (->rat y)))
    (make-rat (+ (* (rat-num a) (rat-den b))
                 (* (rat-num b) (rat-den a)))
              (* (rat-den a) (rat-den b)))))

(define (rat- x y)
  (let ((a (->rat x)) (b (->rat y)))
    (make-rat (- (* (rat-num a) (rat-den b))
                 (* (rat-num b) (rat-den a)))
              (* (rat-den a) (rat-den b)))))

; a/b * c/d = ac/bd
(define (rat* x y)
  (let ((a (->rat x)) (b (->rat y)))
    (make-rat (* (rat-num a) (rat-num b))
              (* (rat-den a) (rat-den b)))))

; a/b / c/d = ad/bc
(define (rat/ x y)
  (let ((a (->rat x)) (b (->rat y)))
    (make-rat (* (rat-num a) (rat-den b))
              (* (rat-den a) (rat-num b)))))

; ---- comparison ----
(define (rat= x y)
  (let ((a (->rat x)) (b (->rat y)))
    (and (= (rat-num a) (rat-num b))
         (= (rat-den a) (rat-den b)))))

(define (rat< x y)
  (let ((a (->rat x)) (b (->rat y)))
    (< (* (rat-num a) (rat-den b))
       (* (rat-num b) (rat-den a)))))

; ---- conversion / display ----
(define (rat->string r)
  (if (= (rat-den r) 1)
      (number->string (rat-num r))
      (string-append (number->string (rat-num r))
                     "/"
                     (number->string (rat-den r)))))

; ---- helpers ----
(define (rat-negate r)
  (make-rat (- 0 (rat-num r)) (rat-den r)))

(define (rat-reciprocal r)
  (make-rat (rat-den r) (rat-num r)))

; Sum a list of rationals.
(define (rat-sum lst)
  (fold-left rat+ (make-rat 0 1) lst))

; Product of a list of rationals.
(define (rat-product lst)
  (fold-left rat* (make-rat 1 1) lst))

; ---- harmonic / common series ----
; Harmonic number H(n) = 1 + 1/2 + 1/3 + ... + 1/n  (exact!)
(define (harmonic n)
  (define (go k acc)
    (if (> k n) acc
      (go (+ k 1) (rat+ acc (make-rat 1 k)))))
  (go 1 (make-rat 0 1)))
