(define (safe-divide x y)
  (call/cc (lambda (escape)
    (if (= y 0)
        (escape "Error: Division by zero")
        (/ x y)))))

(safe-divide 10 0)

