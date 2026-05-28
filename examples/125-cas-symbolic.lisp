; -*- lisp -*-
; ============================================================
;  EXAMPLE 125 — A symbolic CAS (differentiation & simplification)
; ============================================================

(import "match.lisp")
(import "cas.lisp")

(display "=== SIMPLIFICATION ===") (newline)
(display "  (+ x 0)       → ") (display (cas->string (simplify '(+ x 0)))) (newline)
(display "  (* 1 x)       → ") (display (cas->string (simplify '(* 1 x)))) (newline)
(display "  (* x 0)       → ") (display (cas->string (simplify '(* x 0)))) (newline)
(display "  (^ x 1)       → ") (display (cas->string (simplify '(^ x 1)))) (newline)
(display "  (+ (* 2 3) 0) → ") (display (cas->string (simplify '(+ (* 2 3) 0)))) (newline)
(newline)

(display "=== DIFFERENTIATION ===") (newline)
(display "  d/dx c        = ") (display (cas->string (derivative 5 'x))) (newline)
(display "  d/dx x        = ") (display (cas->string (derivative 'x 'x))) (newline)
(display "  d/dx x²       = ") (display (cas->string (derivative '(^ x 2) 'x))) (newline)
(display "  d/dx x³       = ") (display (cas->string (derivative '(^ x 3) 'x))) (newline)
(display "  d/dx (x²+x)   = ") (display (cas->string (derivative '(+ (^ x 2) x) 'x))) (newline)
(display "  d/dx (x·x)    = ") (display (cas->string (derivative '(* x x) 'x))) (newline)
(newline)

(display "=== PRODUCT & QUOTIENT RULES ===") (newline)
(display "  d/dx (x·sin x) = ")
(display (cas->string (derivative '(* x (sin x)) 'x))) (newline)
(display "  d/dx (1/x)     = ")
(display (cas->string (derivative '(/ 1 x) 'x))) (newline)
(newline)

(display "=== ELEMENTARY FUNCTIONS ===") (newline)
(display "  d/dx sin x = ") (display (cas->string (derivative '(sin x) 'x))) (newline)
(display "  d/dx cos x = ") (display (cas->string (derivative '(cos x) 'x))) (newline)
(display "  d/dx eˣ    = ") (display (cas->string (derivative '(exp x) 'x))) (newline)
(display "  d/dx ln x  = ") (display (cas->string (derivative '(ln x) 'x))) (newline)
(newline)

(display "=== CHAIN RULE ===") (newline)
(display "  d/dx sin(x²) = ")
(display (cas->string (derivative '(sin (^ x 2)) 'x))) (newline)
(newline)

(display "=== INTEGRATION (polynomial cases) ===") (newline)
(display "  ∫ c dx   = ") (display (cas->string (integrate 5 'x))) (newline)
(display "  ∫ x dx   = ") (display (cas->string (integrate 'x 'x))) (newline)
(display "  ∫ x² dx  = ") (display (cas->string (integrate '(^ x 2) 'x))) (newline)
(display "  ∫ x³ dx  = ") (display (cas->string (integrate '(^ x 3) 'x))) (newline)
(display "  ∫(x²+x)dx= ") (display (cas->string (integrate '(+ (^ x 2) x) 'x))) (newline)
(newline)

(display "=== End of symbolic CAS ===") (newline)
