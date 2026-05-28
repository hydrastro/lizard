; -*- lisp -*-
; lib/church.lisp — Church encodings: data as pure functions.
;
; Demonstrates that all data (numbers, booleans, pairs, lists)
; can be encoded using only lambda abstractions. This is the
; theoretical foundation of the lambda calculus.

; ============================================================
;  CHURCH BOOLEANS
; ============================================================
; true  = λt.λf. t   (select first)
; false = λt.λf. f   (select second)

(define church-true  (lambda (t) (lambda (f) t)))
(define church-false (lambda (t) (lambda (f) f)))

; if b then x else y  =  ((b x) y)
(define (church-if b x y) ((b x) y))

; and = λp.λq. p q p
(define (church-and p q) ((p q) p))
; or  = λp.λq. p p q
(define (church-or p q) ((p p) q))
; not = λp. p false true
(define (church-not p) ((p church-false) church-true))

; Convert Church boolean to native boolean
(define (church->bool b) ((b #t) #f))

; ============================================================
;  CHURCH NUMERALS
; ============================================================
; n = λf.λx. f^n(x)   (apply f to x, n times)
; 0 = λf.λx. x
; 1 = λf.λx. f x
; 2 = λf.λx. f (f x)

(define church-zero (lambda (f) (lambda (x) x)))

; successor: λn.λf.λx. f ((n f) x)
(define (church-succ n)
  (lambda (f) (lambda (x) (f ((n f) x)))))

; addition: λm.λn.λf.λx. (m f) ((n f) x)
(define (church-add m n)
  (lambda (f) (lambda (x) ((m f) ((n f) x)))))

; multiplication: λm.λn.λf. m (n f)
(define (church-mul m n)
  (lambda (f) (m (n f))))

; exponentiation: λm.λn. n m
(define (church-exp m n) (n m))

; Convert Church numeral to native integer
(define (church->int n)
  ((n (lambda (x) (+ x 1))) 0))

; Convert native integer to Church numeral
(define (int->church n)
  (if (= n 0) church-zero
    (church-succ (int->church (- n 1)))))

; ============================================================
;  CHURCH PAIRS
; ============================================================
; pair = λx.λy.λf. f x y
; fst  = λp. p (λx.λy. x)
; snd  = λp. p (λx.λy. y)

(define (church-pair x y) (lambda (f) ((f x) y)))
(define (church-fst p) (p (lambda (x) (lambda (y) x))))
(define (church-snd p) (p (lambda (x) (lambda (y) y))))

; ============================================================
;  CHURCH LISTS (via right fold encoding)
; ============================================================
; nil  = λc.λn. n
; cons = λh.λt.λc.λn. c h (t c n)

(define church-nil (lambda (c) (lambda (n) n)))
(define (church-cons h t)
  (lambda (c) (lambda (n) ((c h) ((t c) n)))))

; sum a church list of church numerals  
(define (church-list-sum lst)
  ((lst (lambda (h) (lambda (acc) (church-add h acc)))) church-zero))
