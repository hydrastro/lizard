(import "cas/tower.lisp")
(define LOG (list 'log))
(define (R1)(rat-one))(define (R0)(rat-zero))(define (negone)(rat-neg (rat-one)))
; recover 1/(L^2+1) from its derivative
(define g1 (tr-make (rf-const (R1)) (list (R1)(R0)(R1))))
(define f1 (tr-deriv g1 LOG))
(define r1 (integrate-proper (car f1)(car (cdr f1)) LOG))
(display (tr->string (car (cdr r1)) "L"))(display "  cert=")(display (proper-verify (car f1)(car (cdr f1)) LOG r1))(newline)
; base case 2x/(x^2+1) -> log
(define r2 (integrate-proper (rf-const (rat-from-poly (list 0 2)))(rf-const (rat-from-poly (list 1 0 1))) LOG))
(display "log(")(display (rfpoly->string (car (cdr (car (cdr (cdr r2))))) "L"))(display ")  cert=")
(display (proper-verify (rf-const (rat-from-poly (list 0 2)))(rf-const (rat-from-poly (list 1 0 1))) LOG r2))(newline)
