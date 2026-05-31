; csp_select.lisp — golden test for cooperative concurrency (select/fork/deadlock).
; All printed values are scheduling-independent (sums, counts, verdict).
(import "csp-select.lisp")

(define (tagged results tag)
  (if (null? results) #f
    (if (and (pair? (car results)) (equal? (car (car results)) tag))
        (car (cdr (car results)))
        (tagged (cdr results) tag))))

; fork/join: 4 workers send i*i, collector sums -> 30
(define jres (co-channel))
(define (jworker i) (lambda () (co-send jres (* i i) (lambda () (co-done 'w)))))
(define (jcollect got total)
  (if (= got 4) (co-done (list 'sum total))
    (co-recv jres (lambda (v) (jcollect (+ got 1) (+ total v))))))
(define (jboss)
  (co-fork (jworker 1) (lambda () (co-fork (jworker 2)
   (lambda () (co-fork (jworker 3) (lambda () (co-fork (jworker 4)
    (lambda () (jcollect 0 0))))))))))
(define fork-r (csp-run (list jboss) 5000))
(display "fork-join-status ") (display (run-status fork-r)) (newline)
(display "fork-join-sum ") (display (tagged (run-results fork-r) 'sum)) (newline)

; select fan-in (merger first -> select parks then wakes): sum 66
(define a (co-channel)) (define b (co-channel))
(define (prod ch xs)
  (define (loop ys) (if (null? ys) (co-done 'p)
    (co-send ch (car ys) (lambda () (loop (cdr ys))))))
  (lambda () (loop xs)))
(define (merger x y n)
  (define (loop got total) (if (= got n) (co-done (list 'sum total))
    (co-select (list (alt-recv x (lambda (v) (loop (+ got 1) (+ total v))))
                     (alt-recv y (lambda (v) (loop (+ got 1) (+ total v))))))))
  (lambda () (loop 0 0)))
(define sel-r (csp-run (list (merger a b 6) (prod a (list 1 2 3)) (prod b (list 10 20 30))) 5000))
(display "select-status ") (display (run-status sel-r)) (newline)
(display "select-sum ") (display (tagged (run-results sel-r) 'sum)) (newline)

; deadlock: two receivers, no senders
(define c (co-channel)) (define d (co-channel))
(define dl-r (csp-run (list (lambda () (co-recv c (lambda (v) (co-done v))))
                            (lambda () (co-recv d (lambda (v) (co-done v))))) 5000))
(display "deadlock-status ") (display (run-status dl-r)) (newline)
(display "deadlock-blocked ") (display (run-blocked dl-r)) (newline)
