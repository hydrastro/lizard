; -*- lisp -*-
; lib/control.lisp — Control-flow forms (Evolution Plan Phase 4).
;
; Since lizard evaluates arguments eagerly, forms that need
; short-circuiting take THUNKS (zero-arg lambdas) for their bodies.

; ============================================================
;  CASE — dispatch on a value
; ============================================================
; clauses is a list of (key-list handler-thunk). The value is
; compared against each key in key-list; the first match wins.
; An 'else key-list always matches.
;
;   (case-of 2
;     (list
;       (list '(1) (lambda () "one"))
;       (list '(2 3) (lambda () "two or three"))
;       (list 'else (lambda () "other"))))

(define (case-of val clauses)
  (if (null? clauses)
      #f
      (let ((clause (car clauses)))
        (let ((keys (car clause))
              (handler (car (cdr clause))))
          (if (equal? keys 'else)
              (handler)
              (if (member val keys)
                  (handler)
                  (case-of val (cdr clauses))))))))

; ============================================================
;  WHEN / UNLESS (thunk-based, short-circuiting)
; ============================================================

(define (when-do test thunk)
  (if test (thunk) '()))

(define (unless-do test thunk)
  (if test '() (thunk)))

; ============================================================
;  COND-style multi-branch (thunk-based)
; ============================================================
; clauses is a list of (test-thunk body-thunk). Returns the body
; of the first clause whose test is truthy.

(define (cond-of clauses)
  (if (null? clauses)
      '()
      (let ((clause (car clauses)))
        (let ((test (car clause))
              (body (car (cdr clause))))
          (if (equal? test 'else)
              (body)
              (if (test)
                  (body)
                  (cond-of (cdr clauses))))))))

; ============================================================
;  LOOPS
; ============================================================

; Repeat a thunk n times, collecting results.
(define (dotimes n thunk)
  (define (go i acc)
    (if (= i n) (reverse acc)
      (go (+ i 1) (cons (thunk) acc))))
  (go 0 '()))

; While loop: run body-thunk while test-thunk is truthy.
; Returns the number of iterations.
(define (while-do test-thunk body-thunk)
  (define (go count)
    (if (test-thunk)
        (begin (body-thunk) (go (+ count 1)))
        count))
  (go 0))

; Run a thunk and return both its result and a label.
(define (labeled label thunk)
  (cons label (thunk)))

; ============================================================
;  PIPELINE / THREADING (value-based)
; ============================================================

; Apply a sequence of single-arg functions left to right.
(define (thread-through val fns)
  (fold-left (lambda (acc f) (f acc)) val fns))

; Conditional application: apply f to val only if pred holds.
(define (apply-when pred f val)
  (if (pred val) (f val) val))
