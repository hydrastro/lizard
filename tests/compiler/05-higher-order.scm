(define (mapf f l) (if (null? l) (quote ()) (cons (f (car l)) (mapf f (cdr l)))))
(define (foldl f acc l) (if (null? l) acc (foldl f (f acc (car l)) (cdr l))))
(display (mapf (lambda (x) (* x x)) (quote (1 2 3 4 5)))) (newline)
(display (foldl + 0 (quote (1 2 3 4 5 6 7 8 9 10)))) (newline)
