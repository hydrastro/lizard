; -*- lisp -*-
; ============================================================
;  EXAMPLE 122 — Track R: a reader with #lang dialects
; ============================================================

(import "match.lisp")
(import "reader.lisp")

(display "=== TOKENIZER ===") (newline)
(display "  tokenize \"(+ 1 (* 2 3))\":") (newline)
(display "    ") (display (tokenize "(+ 1 (* 2 3))")) (newline)
(newline)

(display "=== READER (text -> s-expression) ===") (newline)
(display "  read \"(+ 1 2)\": ") (display (read-str "(+ 1 2)")) (newline)
(display "  read \"(f (g x) y)\": ") (display (read-str "(f (g x) y)")) (newline)
(display "  read nested \"(a (b (c d)))\": ")
(display (read-str "(a (b (c d)))")) (newline)
(newline)

(display "=== NUMBERS vs SYMBOLS ===") (newline)
(display "  read \"(list 1 two 3 four)\": ")
(display (read-str "(list 1 two 3 four)")) (newline)
(display "  (1 and 3 became numbers; two/four stayed symbols)") (newline)
(newline)

(display "=== READ ALL TOP-LEVEL FORMS ===") (newline)
(display "  read-all \"(define x 1) (define y 2)\":") (newline)
(display "    ") (display (read-all "(define x 1) (define y 2)")) (newline)
(newline)

(display "=== #lang DIALECTS ===") (newline)
(define reg (dialect-registry identity-dialect infix-dialect quote-all-dialect))

(display "  Program with (#lang infix):") (newline)
(define infix-src "(#lang infix) (1 + 2) (3 * 4)")
(display "    source: ") (display infix-src) (newline)
(display "    after #lang infix transform:") (newline)
(display "      ") (display (read-program infix-src reg)) (newline)
(display "    (infix (1 + 2) rewritten to prefix (+ 1 2))") (newline)
(newline)

(display "  Program with (#lang quoted):") (newline)
(define q-src "(#lang quoted) (a b) (c d)")
(display "    after transform: ") (display (read-program q-src reg)) (newline)
(newline)

(display "  Program with (#lang plain):") (newline)
(define p-src "(#lang plain) (f x) (g y)")
(display "    after transform: ") (display (read-program p-src reg)) (newline)
(newline)

(display "=== Track R COMPLETE: reader + #lang ✓ ===") (newline)
