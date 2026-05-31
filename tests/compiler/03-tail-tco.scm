(define (loop n acc) (if (= n 0) acc (loop (- n 1) (+ acc 1))))
(display (loop 500000 0)) (newline)
(define (down n) (if (= n 0) (quote done) (down (- n 1))))
(display (down 500000)) (newline)
