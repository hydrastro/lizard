(define (my-even? n) (if (= n 0) #t (my-odd? (- n 1))))
(define (my-odd? n) (if (= n 0) #f (my-even? (- n 1))))
(display (my-even? 100)) (display " ") (display (my-odd? 100)) (newline)
(display (my-even? 7)) (display " ") (display (my-odd? 7)) (newline)
