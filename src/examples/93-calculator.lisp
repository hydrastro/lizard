; -*- lisp -*-
; ============================================================
;  EXAMPLE 93 — A calculator written in lizard
; ============================================================
;
; This demonstrates lizard's self-sufficiency: a complete little
; expression interpreter, written entirely in lizard.

(import "calc.lisp")

(display "=== Literals ===") (newline)
(calc-run "42" 42)
(newline)

(display "=== Arithmetic ===") (newline)
(calc-run "(+ 2 3)" '(+ 2 3))
(calc-run "(* 6 7)" '(* 6 7))
(calc-run "(- 100 58)" '(- 100 58))
(calc-run "(/ 84 2)" '(/ 84 2))
(newline)

(display "=== Nested expressions ===") (newline)
(calc-run "(+ (* 2 3) (* 4 5))" '(+ (* 2 3) (* 4 5)))
(calc-run "(* (+ 1 2) (+ 3 4))" '(* (+ 1 2) (+ 3 4)))
(calc-run "(- (* 10 10) (* 3 3))" '(- (* 10 10) (* 3 3)))
(newline)

(display "=== Variables with let ===") (newline)
(calc-run "(let x 10 (+ x x))" '(let x 10 (+ x x)))
(calc-run "(let x 5 (let y 3 (* x y)))" '(let x 5 (let y 3 (* x y))))
(calc-run "(let a 2 (let b 3 (let c 4 (+ a (* b c)))))"
          '(let a 2 (let b 3 (let c 4 (+ a (* b c))))))
(newline)

(display "=== Complex computation ===") (newline)
; Compute (x^2 + y^2) where x=3, y=4  → 25
(calc-run "x²+y² (x=3,y=4)"
          '(let x 3 (let y 4 (+ (* x x) (* y y)))))
; Quadratic: ax² + bx + c  where a=1,b=2,c=3,x=5  → 25+10+3 = 38
(calc-run "ax²+bx+c (a=1,b=2,c=3,x=5)"
          '(let a 1 (let b 2 (let c 3 (let x 5
             (+ (+ (* a (* x x)) (* b x)) c))))))
(newline)

(display "╔═══════════════════════════════════════════════╗") (newline)
(display "║  A full interpreter, written in lizard,       ║") (newline)
(display "║  running on lizard. Self-sufficiency proven.  ║") (newline)
(display "╚═══════════════════════════════════════════════╝") (newline)
