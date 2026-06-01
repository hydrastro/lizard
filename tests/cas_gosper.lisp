(import "cas/gosper.lisp")
(define (G rnum rden) (display (gosper-result->string (gosper-sum rnum rden))) (newline))
(G (list 1 1) (list 0 1))            ; SUM k
(G (list 1 2 1) (list 0 0 1))        ; SUM k^2
(G (list 1 2 1) (list 0 1))          ; SUM k*k!  -> (1/n) t
(G (list 0 1) (list 2 1))            ; SUM 1/(n(n+1))
(G (list 0 1) (list 1 1))            ; SUM 1/n -> not summable
(G (list 2 4) (list 1 1))            ; SUM C(2n,n) -> not summable
(G (poly-pow (list 1 1) 3) (list 0 0 1)) ; SUM n^2 n! -> not summable
