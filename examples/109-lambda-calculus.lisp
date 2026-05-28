; -*- lisp -*-
; ============================================================
;  EXAMPLE 109 — Lambda calculus in lizard
; ============================================================

(import "lambda-calc.lisp")

(display "=== IDENTITY & APPLICATION ===") (newline)
; (λx.x) y → y
(define id '(lam x x))
(display "  (λx.x) y → ")
(display (lc-normalize (list 'app id 'y) 100)) (newline)
(newline)

(display "=== K combinator (const) ===") (newline)
; K = λx.λy.x ;  K a b → a
(define K '(lam x (lam y x)))
(display "  K a b → ")
(display (lc-normalize (list 'app (list 'app K 'a) 'b) 100)) (newline)
(newline)

(display "=== CAPTURE-AVOIDING SUBSTITUTION ===") (newline)
; (λx.λy.x) y  must NOT capture the free y — alpha-renaming kicks in
(display "  (λx.λy.x) y → ")
(display (lc-normalize (list 'app '(lam x (lam y x)) 'y) 100)) (newline)
(display "  (the inner y is renamed to avoid capture)") (newline)
(newline)

(display "=== FREE VARIABLES ===") (newline)
(display "  free vars of (λx. (app x y)): ")
(display (free-vars '(lam x (app x y)))) (newline)
(display "  free vars of (app (λx.x) z): ")
(display (free-vars '(app (lam x x) z))) (newline)
(newline)

(display "=== CHURCH NUMERALS ===") (newline)
(display "  church 0 = ") (display (church-term 0)) (newline)
(display "  church 2 = ") (display (church-term 2)) (newline)
(display "  decode (church 3) = ") (display (church-decode (church-term 3))) (newline)
(newline)

(display "=== CHURCH ARITHMETIC (pure beta reduction!) ===") (newline)
; succ 2 = 3
(define succ-2 (list 'app church-succ (church-term 2)))
(display "  succ 2 = ") (display (church-decode (lc-normalize succ-2 1000))) (newline)

; plus 2 3 = 5
(define plus-2-3
  (list 'app (list 'app church-plus (church-term 2)) (church-term 3)))
(display "  plus 2 3 = ") (display (church-decode (lc-normalize plus-2-3 1000))) (newline)

; mult 3 4 = 12
(define mult-3-4
  (list 'app (list 'app church-mult (church-term 3)) (church-term 4)))
(display "  mult 3 4 = ") (display (church-decode (lc-normalize mult-3-4 2000))) (newline)
(newline)

(display "=== REDUCTION STEP COUNTS ===") (newline)
(display "  steps to normalize (plus 2 3): ")
(display (lc-steps plus-2-3 1000)) (newline)
(display "  steps to normalize (mult 3 4): ")
(display (lc-steps mult-3-4 2000)) (newline)
(newline)

(display "╔══════════════════════════════════════════════════╗") (newline)
(display "║  Arithmetic computed by pure beta reduction —     ║") (newline)
(display "║  the lambda calculus, fully operational.          ║") (newline)
(display "╚══════════════════════════════════════════════════╝") (newline)
