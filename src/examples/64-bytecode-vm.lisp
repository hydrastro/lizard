; -*- lisp -*-
; ============================================================
;  EXAMPLE 64 — Bytecode compiler + VM (Phase E)
; ============================================================
;
; lizard now has a bytecode compiler and stack-based VM.
;
;   (vm-eval expr)       — compile and execute on the VM
;   (disassemble expr)   — show the compiled bytecode
;
; The compiler handles: constants, arithmetic, comparisons,
; variables, if/else, define, lambda, function calls, begin,
; cons/car/cdr, display/newline.
;
; Forms not supported by the compiler (macros, call/cc, type
; theory nodes) fall back to being stored as constants.

(display "=== Arithmetic ===") (newline)
(display "  (vm-eval '(+ 2 3)): ")
(display (vm-eval '(+ 2 3))) (newline)
(display "  (vm-eval '(* 6 7)): ")
(display (vm-eval '(* 6 7))) (newline)
(display "  (vm-eval '(+ (* 3 4) (- 10 2))): ")
(display (vm-eval '(+ (* 3 4) (- 10 2)))) (newline)
(newline)

(display "=== Conditionals ===") (newline)
(display "  (vm-eval '(if (> 5 3) 'yes 'no)): ")
(display (vm-eval '(if (> 5 3) 'yes 'no))) (newline)
(display "  (vm-eval '(if (< 5 3) 'yes 'no)): ")
(display (vm-eval '(if (< 5 3) 'yes 'no))) (newline)
(newline)

(display "=== Disassembly ===") (newline)
(disassemble '(+ (* 3 4) (- 10 2)))
(newline)

(display "=== Lambda + function calls ===") (newline)
(display "  (vm-eval '((lambda (x) (* x x)) 7)): ")
(display (vm-eval '((lambda (x) (* x x)) 7))) (newline)
(newline)

(display "=== End of bytecode VM example ===") (newline)
