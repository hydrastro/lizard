; -*- lisp -*-
; lib/test.lisp — A minimal unit testing framework for lizard.
;
; Usage:
;   (import "test.lisp")
;   (test-begin "my suite")
;   (check-equal? (+ 1 2) 3 "addition works")
;   (check-true (even? 4) "four is even")
;   (test-end)

; State is held in atoms (mutable cells).
(define test-passed (atom 0))
(define test-failed (atom 0))
(define test-suite-name (atom ""))

(define (test-begin name)
  (reset! test-passed 0)
  (reset! test-failed 0)
  (reset! test-suite-name name)
  (display "Running suite: ")
  (display name)
  (newline))

(define (test-end)
  (newline)
  (display "Results: ")
  (display (deref test-passed))
  (display " passed, ")
  (display (deref test-failed))
  (display " failed.")
  (newline)
  (if (= (deref test-failed) 0)
      (display "  ALL TESTS PASSED")
      (display "  SOME TESTS FAILED"))
  (newline))

(define (test-pass! msg)
  (swap! test-passed (lambda (n) (+ n 1)))
  (display "  PASS  ")
  (display msg)
  (newline))

(define (test-fail! msg expected actual)
  (swap! test-failed (lambda (n) (+ n 1)))
  (display "  FAIL  ")
  (display msg)
  (display " — expected ")
  (display expected)
  (display ", got ")
  (display actual)
  (newline))

; ---- assertions ----

(define (check-equal? actual expected msg)
  (if (equal? actual expected)
      (test-pass! msg)
      (test-fail! msg expected actual)))

(define (check-true cond msg)
  (if cond
      (test-pass! msg)
      (test-fail! msg #t #f)))

(define (check-false cond msg)
  (if cond
      (test-fail! msg #f #t)
      (test-pass! msg)))

(define (check-not-equal? actual unexpected msg)
  (if (equal? actual unexpected)
      (test-fail! msg (string-append "not " (number->string unexpected)) actual)
      (test-pass! msg)))

; Check that a predicate holds for all elements
(define (check-all pred lst msg)
  (if (fold-left (lambda (acc x) (and acc (pred x))) #t lst)
      (test-pass! msg)
      (test-fail! msg "all true" "some false")))
