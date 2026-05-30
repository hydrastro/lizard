; -*- lisp -*-
; ============================================================
;  EXAMPLE 95b — Structural pattern matching (Phase 4)
; ============================================================

(import "match.lisp")
(import "pattern.lisp")

(display "=== BASIC PATTERNS ===") (newline)

; Wildcard
(display "  match 42 against _: ")
(display (match-pattern '_ 42)) (newline)

; Variable binding
(display "  match 42 against x: ")
(display (match-pattern 'x 42)) (newline)

; Literal match (success)
(display "  match 42 against 42: ")
(display (match-pattern 42 42)) (newline)

; Literal match (failure)
(display "  match 42 against 99: ")
(display (match-pattern 99 42)) (newline)
(newline)

(display "=== PAIR DESTRUCTURING ===") (newline)
(display "  match (1 . 2) against (cons a b): ")
(display (match-pattern '(cons a b) (cons 1 2))) (newline)
(newline)

(display "=== LIST DESTRUCTURING ===") (newline)
(display "  match (1 2 3) against (list a b c): ")
(display (match-pattern '(list a b c) '(1 2 3))) (newline)
(display "  match (1 2) against (list a b c) [wrong length]: ")
(display (match-pattern '(list a b c) '(1 2))) (newline)
(newline)

(display "=== MATCH RUNNER ===") (newline)
; Classify a value
(define (classify val)
  (match-run val
    (list
      (list 0 (lambda (b) "zero"))
      (list '(cons h t) (lambda (b) "non-empty list"))
      (list '_ (lambda (b) "something else")))))

(display "  classify 0: ") (display (classify 0)) (newline)
(display "  classify (1 2 3): ") (display (classify '(1 2 3))) (newline)
(display "  classify \"hi\": ") (display (classify "hi")) (newline)
(newline)

(display "=== EXTRACTING BINDINGS ===") (newline)
; Use bindings in the handler
(define (describe-pair val)
  (match-run val
    (list
      (list '(cons first second)
            (lambda (b)
              (list 'pair
                    (binding-ref 'first b)
                    (binding-ref 'second b))))
      (list '_ (lambda (b) 'not-a-pair)))))

(display "  describe (10 . 20): ")
(display (describe-pair (cons 10 20))) (newline)
(display "  describe 5: ")
(display (describe-pair 5)) (newline)
(newline)

(display "=== RECURSIVE: list length via match ===") (newline)
(define (my-length lst)
  (match-run lst
    (list
      (list '() (lambda (b) 0))
      (list '(cons h t)
            (lambda (b) (+ 1 (my-length (binding-ref 't b))))))))
(display "  length of (a b c d e): ")
(display (my-length '(a b c d e))) (newline)
(newline)

(display "=== Phase 4 milestone: structural match ✓ ===") (newline)
