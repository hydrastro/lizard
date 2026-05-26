(call/cc (lambda (exit)
  (let loop ((x 0))
    (if (> x 3)
        (exit x)
        (loop (+ x 1))))))


(+ 1 (call/cc (lambda (k) (k 10))))


