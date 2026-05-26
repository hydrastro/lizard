; -*- lisp -*-
; Macros via define-syntax + lambda + quasiquote.

; Quasiquote builds a list literal with ,holes you can unquote into.
`(a ,(+ 1 1) c)           ; => (a 2 c)

; Unquote-splicing pastes a list into the surrounding list.
(define xs '(b c d))
`(a ,@xs e)               ; => (a b c d e)

; A macro is a lambda whose result is interpreted as code.

(define-syntax my-when
  (lambda (test body)
    `(if ,test ,body #f)))

(my-when #t 42)           ; => 42
(my-when #f 42)           ; => #f
(my-when (= 1 1) (+ 100 23))    ; => 123

(define-syntax my-unless
  (lambda (test body)
    `(if ,test #f ,body)))

(my-unless #t 99)         ; => #f
(my-unless #f 99)         ; => 99

; A macro that builds an addition.
(define-syntax add-one
  (lambda (x)
    `(+ ,x 1)))

(add-one 41)              ; => 42

; A macro that builds a let. Useful for short-lived bindings.
(define-syntax with
  (lambda (name value body)
    `(let ((,name ,value)) ,body)))

(with x 10 (* x x))       ; => 100
