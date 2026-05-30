; -*- lisp -*-
; lib/matrix.lisp — Matrix operations (linear algebra).
;
; A matrix is a list of rows, each row a list of numbers.
; Example: ((1 2 3) (4 5 6)) is a 2x3 matrix.
;
; NOTE: lizard's `map` takes a single list, so we define `map2`
; for element-wise binary operations.

; ---- map2: element-wise binary map over two lists ----
(define (map2 f a b)
  (if (or (null? a) (null? b)) '()
    (cons (f (car a) (car b))
          (map2 f (cdr a) (cdr b)))))

; ---- construction ----
(define (matrix-build rows cols f)
  (map (lambda (i)
         (map (lambda (j) (f i j))
              (range 0 cols)))
       (range 0 rows)))

(define (matrix-identity n)
  (matrix-build n n (lambda (i j) (if (= i j) 1 0))))

(define (matrix-zero rows cols)
  (matrix-build rows cols (lambda (i j) 0)))

; ---- accessors ----
(define (matrix-rows m) (length m))
(define (matrix-cols m) (if (null? m) 0 (length (car m))))
(define (matrix-ref m i j) (list-ref (list-ref m i) j))

; ---- operations ----
(define (matrix-add a b)
  (map2 (lambda (ra rb) (map2 + ra rb)) a b))

(define (matrix-sub a b)
  (map2 (lambda (ra rb) (map2 - ra rb)) a b))

(define (matrix-scale s m)
  (map (lambda (row) (map (lambda (x) (* s x)) row)) m))

(define (matrix-transpose m)
  (if (null? (car m)) '()
    (cons (map car m)
          (matrix-transpose (map cdr m)))))

(define (dot-product a b)
  (fold-left + 0 (map2 * a b)))

(define (matrix-mul a b)
  (let ((bt (matrix-transpose b)))
    (map (lambda (row)
           (map (lambda (col) (dot-product row col)) bt))
         a)))

(define (matrix-trace m)
  (fold-left + 0
    (map (lambda (i) (matrix-ref m i i))
         (range 0 (matrix-rows m)))))

; ---- vector operations ----
(define (vec-add a b) (map2 + a b))
(define (vec-sub a b) (map2 - a b))
(define (vec-scale s v) (map (lambda (x) (* s x)) v))
(define (vec-norm-sq v) (dot-product v v))

; ---- pretty printing ----
(define (matrix-print m)
  (for-each (lambda (row)
              (display "  ")
              (display row)
              (newline))
            m))
