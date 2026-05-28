(define (power-tail a b) (iter a b 1))
(define (iter a b acc) (if (= b 0) acc (iter a (- b 1) (* acc a))))
(power-tail 123 12312)
