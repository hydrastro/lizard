; -*- lisp -*-
; lib/trace.lisp — Function tracing & debugging (Phase 8).
;
; Wrap functions to log their calls and returns with indentation,
; producing a call tree. Useful for understanding recursion.

; Shared indentation depth (an atom).
(define trace-depth (atom 0))

(define (trace-indent)
  (define (go n)
    (if (<= n 0) ""
      (string-append "| " (go (- n 1)))))
  (go (deref trace-depth)))

(define (trace-enter name args)
  (display (trace-indent))
  (display "→ ")
  (display name)
  (display " ")
  (display args)
  (newline)
  (swap! trace-depth (lambda (n) (+ n 1))))

(define (trace-exit name result)
  (swap! trace-depth (lambda (n) (- n 1)))
  (display (trace-indent))
  (display "← ")
  (display name)
  (display " = ")
  (display result)
  (newline)
  result)

; ============================================================
;  WRAPPING (manual)
; ============================================================
; Wrap a 1-arg function so calls are traced.

(define (trace-fn1 name f)
  (lambda (x)
    (trace-enter name (list x))
    (trace-exit name (f x))))

(define (trace-fn2 name f)
  (lambda (x y)
    (trace-enter name (list x y))
    (trace-exit name (f x y))))

; ============================================================
;  COUNTING — how many times is a function called?
; ============================================================

(define (make-call-counter)
  (atom 0))

(define (counted name counter f)
  (lambda (x)
    (swap! counter (lambda (n) (+ n 1)))
    (f x)))

; ============================================================
;  TIMING-STYLE STEP COUNTER (deterministic "cost" model)
; ============================================================
; lizard has no wall clock here, so we count reduction steps via
; an atom incremented on each traced call. This gives a portable,
; deterministic performance proxy.

(define step-count (atom 0))

(define (reset-steps!) (reset! step-count 0))
(define (tick-step!) (swap! step-count (lambda (n) (+ n 1))))
(define (get-steps) (deref step-count))

; Run a thunk and report how many steps it took.
(define (measure-steps label thunk)
  (reset-steps!)
  (let ((result (thunk)))
    (display "  ")
    (display label)
    (display ": result = ")
    (display result)
    (display ", steps = ")
    (display (get-steps))
    (newline)
    result))

; ============================================================
;  ASSERTIONS / DEBUG OUTPUT
; ============================================================

(define (debug label val)
  (display "  [debug] ")
  (display label)
  (display " = ")
  (display val)
  (newline)
  val)                              ; pass-through for inline use

(define (assert! cond msg)
  (if cond
      #t
      (begin (display "  ASSERTION FAILED: ")
             (display msg)
             (newline)
             #f)))

(define (assert-equal! actual expected msg)
  (if (equal? actual expected)
      #t
      (begin (display "  ASSERTION FAILED: ")
             (display msg)
             (display " (expected ")
             (display expected)
             (display ", got ")
             (display actual)
             (display ")")
             (newline)
             #f)))
