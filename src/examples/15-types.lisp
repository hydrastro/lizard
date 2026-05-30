; -*- lisp -*-
; Reflection, types, and records.
;
; This file demonstrates the type/reflection primitives:
;   (type-of x)         -> symbol naming x's runtime type
;   (defined? 'sym)     -> #t if sym is bound in this env
;   (env-keys)          -> list of every bound symbol in scope
;   (procedure-arity f) -> count of formal params (or 'variadic)
;
; And the string-ops:
;   (string-length s) (string-append a b ...) (substring s i j)
;   (string=? a b)
;   (number->string n) (string->number s)
;   (symbol->string s) (string->symbol s)
;
; And builds records on top of them with no extra C code.

; ---- type-of and reflection ----------------------------------------------

(display "type-of 42         = ") (display (type-of 42))            (newline)
(display "type-of \"hi\"       = ") (display (type-of "hi"))          (newline)
(display "type-of 'foo       = ") (display (type-of 'foo))          (newline)
(display "type-of '(1 2 3)   = ") (display (type-of '(1 2 3)))      (newline)
(display "type-of (cons 1 2) = ") (display (type-of (cons 1 2)))    (newline)
(display "type-of #t         = ") (display (type-of #t))            (newline)
(display "type-of (lambda)   = ") (display (type-of (lambda () 0))) (newline)
(display "type-of +          = ") (display (type-of +))             (newline)
(newline)

; ---- typecheck combinator -----------------------------------------------

(define (type-check expected v)
  (if (= (type-of v) expected)
      v
      (begin (display "TYPE ERROR: expected ") (display expected)
             (display ", got ") (display (type-of v)) (newline)
             '())))

(display "(type-check 'number 42)    = ") (display (type-check 'number 42)) (newline)
(display "(type-check 'number \"hi\")  = ") (display (type-check 'number "hi")) (newline)
(newline)

; ---- Records, built on tagged conses -----------------------------------
;
; A record is represented as: (record TYPE-SYMBOL field1 field2 ... fieldN)
; with the user choosing field names and we map name->index by position.
; Field access and update are O(N) but that's fine for small records.

(define (make-record type-tag fields) (cons 'record (cons type-tag fields)))
(define (record? x)
  (and (pair? x)
       (= (car x) 'record)))
(define (record-type x)
  (if (record? x) (car (cdr x)) 'not-a-record))

; field-nth: positional accessor
(define (field-nth rec n)
  (define (loop xs i)
    (if (= i 0) (car xs) (loop (cdr xs) (- i 1))))
  (loop (cdr (cdr rec)) n))

; Example: a point record with two fields.
(define (make-point x y) (make-record 'point (list x y)))
(define (point-x p) (field-nth p 0))
(define (point-y p) (field-nth p 1))

(define p (make-point 3 4))
(display "p = ")           (display p)               (newline)
(display "record? p = ")   (display (record? p))     (newline)
(display "type? = ")       (display (record-type p)) (newline)
(display "point-x p = ")   (display (point-x p))     (newline)
(display "point-y p = ")   (display (point-y p))     (newline)
(newline)

; Distance helper.
(define (sq n) (* n n))
(define (point-magnitude-sq p) (+ (sq (point-x p)) (sq (point-y p))))
(display "|p|^2 = ") (display (point-magnitude-sq p)) (newline)
(newline)

; ---- Procedure introspection --------------------------------------------

(define (add3 a b c) (+ a b c))
(define (id x) x)
(display "arity add3  = ") (display (procedure-arity add3)) (newline)
(display "arity id    = ") (display (procedure-arity id))   (newline)
(display "arity (k1)  = ") (display (procedure-arity (lambda () 1))) (newline)
(display "arity +     = ") (display (procedure-arity +))    (newline)
(newline)

; ---- A tiny inspect function --------------------------------------------

(define (inspect x)
  (display "{ value: ") (display x)
  (display ", type: ") (display (type-of x))
  (cond ((= (type-of x) 'number)
         (display ", str: ") (display (number->string x)))
        ((= (type-of x) 'string)
         (display ", len: ") (display (string-length x)))
        ((= (type-of x) 'procedure)
         (display ", arity: ") (display (procedure-arity x)))
        ((= (type-of x) 'pair)
         (display ", car-type: ") (display (type-of (car x))))
        (else '()))
  (display " }") (newline))

(inspect 42)
(inspect "lizard")
(inspect 'symbol-here)
(inspect (lambda (a b c) (+ a b c)))
(inspect '(1 2 3))
(inspect #t)
