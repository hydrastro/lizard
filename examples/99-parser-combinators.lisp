; -*- lisp -*-
; ============================================================
;  EXAMPLE 99 — Parser combinators (Phase 3)
; ============================================================

(import "match.lisp")
(import "parser.lisp")

(display "=== PRIMITIVE PARSERS ===") (newline)

; p-lit matches a specific token
(define parse-a (p-lit 'a))
(display "  (p-lit 'a) on (a b c): ")
(let ((r (parse parse-a '(a b c))))
  (display (p-value r)) (display " | rest: ") (display (p-rest r))) (newline)

(display "  (p-lit 'a) on (x y z): ")
(display (p-ok? (parse parse-a '(x y z)))) (display " (fails)") (newline)
(newline)

(display "=== SEQUENCING ===") (newline)
(define parse-ab (p-seq (p-lit 'a) (p-lit 'b)))
(display "  (a then b) on (a b c): ")
(let ((r (parse parse-ab '(a b c))))
  (display (p-value r))) (newline)
(newline)

(display "=== ALTERNATIVES ===") (newline)
(define parse-a-or-b (p-or (p-lit 'a) (p-lit 'b)))
(display "  (a | b) on (a ...): ")
(display (p-value (parse parse-a-or-b '(a x)))) (newline)
(display "  (a | b) on (b ...): ")
(display (p-value (parse parse-a-or-b '(b x)))) (newline)
(newline)

(display "=== REPETITION ===") (newline)
(define parse-many-a (p-many (p-lit 'a)))
(display "  (many a) on (a a a b): ")
(let ((r (parse parse-many-a '(a a a b))))
  (display (p-value r)) (display " | rest: ") (display (p-rest r))) (newline)
(newline)

(display "=== NUMBERS ===") (newline)
(display "  (many1 digit) on (1 2 3 x): ")
(let ((r (parse (p-number) '(1 2 3 x))))
  (display (p-value r))) (newline)
(newline)

(display "=== TRANSFORMATION (p-map) ===") (newline)
; Parse a number list and sum it
(define parse-sum
  (p-map (lambda (digits) (fold-left + 0 digits))
         (p-number)))
(display "  sum of digits (1 2 3 4): ")
(display (p-value (parse parse-sum '(1 2 3 4)))) (newline)
(newline)

(display "=== COMPLETE PARSE ===") (newline)
(display "  parse-all (many a) on (a a a): ")
(display (parse-all (p-many (p-lit 'a)) '(a a a))) (newline)
(display "  parse-all (many a) on (a a b) [leftover]: ")
(display (parse-all (p-many (p-lit 'a)) '(a a b))) (newline)
(newline)

(display "=== A TINY GRAMMAR ===") (newline)
; Grammar: a list of 'x's surrounded by 'open' and 'close'
(define parse-block
  (p-left (p-right (p-lit 'open) (p-many (p-lit 'x)))
          (p-lit 'close)))
(display "  (open x x x close): ")
(display (p-value (parse parse-block '(open x x x close)))) (newline)
(newline)

(display "=== Phase 3 milestone: parser combinators ✓ ===") (newline)
