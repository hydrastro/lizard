(import "cas/risch.lisp")
(define ex (list 0 1)) (define ex2 (list 0 0 1))
(define (E A u) (display (risch-result->string (risch-exp A u) "E")) (newline))
(define (Lg A) (display (risch-result->string (risch-log A) "L")) (newline))
(E (list '() (list 0 1)) ex)        ; INT x e^x
(E (list '() (list 1 0 1)) ex)      ; INT (x^2+1) e^x
(E (list '() (list 0 2)) ex2)       ; INT 2x e^(x^2)
(E (list '() (list 1)) ex2)         ; INT e^(x^2)  -> non-elementary
(E (list '() (list 0 0 1)) ex2)     ; INT x^2 e^(x^2) -> non-elementary
(Lg (list '() (list 1)))            ; INT log x
(Lg (list '() '() (list 1)))        ; INT (log x)^2
(Lg (list '() (list 0 1)))          ; INT x log x
