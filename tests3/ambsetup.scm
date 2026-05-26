(define amb-fail '())

(define (initialize-amb-fail)
  (set! amb-fail
        (lambda (x)
          (error "amb tree exhusted"))))

(define-syntax amb
  (syntax-rules ()
    ((amb e1 e2 ...)
     (let ((old-amb-fail amb-fail))
       (call/cc (lambda (return)
         (call/cc (lambda (next)
           (set! amb-fail next)
           (return e1)))
         (set! amb-fail old-amb-fail)
         (amb-fail #f)))))))

(initialize-amb-fail)


