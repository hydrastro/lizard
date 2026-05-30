; -*- lisp -*-
; ============================================================
;  EXAMPLE 85 — Church encodings: data as pure lambdas
; ============================================================
;
; Everything below is built from NOTHING but lambda abstractions.
; No native numbers, booleans, or pairs are used in the encodings —
; only functions. This is the lambda calculus in action.

(import "church.lisp")

(display "=== CHURCH BOOLEANS ===") (newline)
(display "  church-true  → ") (display (church->bool church-true)) (newline)
(display "  church-false → ") (display (church->bool church-false)) (newline)
(display "  (and true false) → ")
(display (church->bool (church-and church-true church-false))) (newline)
(display "  (or true false)  → ")
(display (church->bool (church-or church-true church-false))) (newline)
(display "  (not true) → ")
(display (church->bool (church-not church-true))) (newline)
(newline)

(display "=== CHURCH NUMERALS ===") (newline)
(display "  zero → ") (display (church->int church-zero)) (newline)
(display "  succ(zero) → ") (display (church->int (church-succ church-zero))) (newline)
(define c2 (int->church 2))
(define c3 (int->church 3))
(display "  2 (as church) → ") (display (church->int c2)) (newline)
(display "  3 (as church) → ") (display (church->int c3)) (newline)
(newline)

(display "  ARITHMETIC ON PURE LAMBDAS:") (newline)
(display "  2 + 3 = ") (display (church->int (church-add c2 c3))) (newline)
(display "  2 * 3 = ") (display (church->int (church-mul c2 c3))) (newline)
(display "  2 ^ 3 = ") (display (church->int (church-exp c2 c3))) (newline)
(define c4 (int->church 4))
(define c5 (int->church 5))
(display "  4 * 5 = ") (display (church->int (church-mul c4 c5))) (newline)
(display "  (2+3) * 4 = ")
(display (church->int (church-mul (church-add c2 c3) c4))) (newline)
(newline)

(display "=== CHURCH PAIRS ===") (newline)
(define p (church-pair 42 99))
(display "  pair(42, 99)") (newline)
(display "  fst → ") (display (church-fst p)) (newline)
(display "  snd → ") (display (church-snd p)) (newline)
(newline)

(display "=== CHURCH LISTS ===") (newline)
(define clist (church-cons c2 (church-cons c3 (church-cons c4 church-nil))))
(display "  list [2,3,4] (church numerals)") (newline)
(display "  sum → ") (display (church->int (church-list-sum clist))) (newline)
(newline)

(display "╔══════════════════════════════════════════════╗") (newline)
(display "║  All arithmetic above used ZERO native ints. ║") (newline)
(display "║  Just lambda abstractions — the foundation    ║") (newline)
(display "║  of computation itself.                       ║") (newline)
(display "╚══════════════════════════════════════════════╝") (newline)
