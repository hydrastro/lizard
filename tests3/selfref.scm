(define (x a) (if (= a '()) 0 (cons (car a) (x (cdr a)))))
(x (list 1 2 3))


