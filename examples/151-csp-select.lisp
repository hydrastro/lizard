; 151-csp-select.lisp — cooperative concurrency: select, fork/join, deadlock.
;
; lizard runs on a single thread (the C89 / -pedantic-errors build admits no
; portable OS threads) and call/cc is escape-only, so the only SOUND
; concurrency model is COOPERATIVE single-threaded scheduling.  A process is a
; step-function returning an EFFECT the scheduler interprets:
;
;     (co-done v)        finished with result v
;     (co-send ch v k)   send v on channel ch, continue with thunk k
;     (co-recv ch k)     receive from ch, continue with (k value)
;     (co-select alts)   CHOOSE among several channel ops (the CSP primitive)
;     (co-fork child k)  spawn `child`, continue with thunk k
;
; Channels are integer handles into one shared registry (the interpreter copies
; compound values placed inside other structures, so a mutable cell carried
; inside an effect would be duplicated; an integer handle copies harmlessly and
; always reaches the single registry).  `csp-run` runs a list of processes to
; completion and returns (ok results), or (deadlock results live) when every
; remaining process is blocked forever — a property the scheduler can report
; precisely because it counts live (spawned-but-unfinished) processes.
;
; Self-checking: each `must` raises on failure (non-zero exit).  Every check is
; a SCHEDULING-INDEPENDENT property (a sum, a count, or the deadlock verdict),
; never a raw interleaving — so the test pins down correctness, not luck.

(import "csp-select.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'csp-check-failed)))

; pull the value out of the first (tag . _) result with the given tag
(define (tagged results tag)
  (if (null? results)
      #f
      (if (and (pair? (car results)) (equal? (car (car results)) tag))
          (car (cdr (car results)))
          (tagged (cdr results) tag))))

(display "cooperative concurrency: select, fork/join, deadlock detection")
(newline) (newline)

; ============================================================
; 1. A two-stage PIPELINE built from send/recv
;    producer -> doubler -> consumer ; consumer sums the doubled values
; ============================================================
(display "Pipeline (producer -> doubler -> consumer):") (newline)
(define p-in  (co-channel))
(define p-out (co-channel))

(define (pipe-producer ch xs)
  (define (loop ys)
    (if (null? ys)
        (co-done 'produced)
        (co-send ch (car ys) (lambda () (loop (cdr ys))))))
  (lambda () (loop xs)))

(define (pipe-doubler in out n)
  (define (loop i)
    (if (= i n)
        (co-done 'doubled)
        (co-recv in (lambda (v)
                      (co-send out (* 2 v) (lambda () (loop (+ i 1))))))))
  (lambda () (loop 0)))

(define (pipe-consumer ch n)
  (define (loop i total)
    (if (= i n)
        (co-done (list 'total total))
        (co-recv ch (lambda (v) (loop (+ i 1) (+ total v))))))
  (lambda () (loop 0 0)))

(define pipe-result
  (csp-run (list (pipe-producer p-in (list 1 2 3 4 5))
                 (pipe-doubler p-in p-out 5)
                 (pipe-consumer p-out 5))
           5000))
(must "pipeline completes (status ok)" (equal? (run-status pipe-result) 'ok))
(must "sum of 2*(1..5) = 30"
      (equal? (tagged (run-results pipe-result) 'total) 30))

; ============================================================
; 2. SELECT fan-in: two producers, a merger that picks whichever
;    channel is ready.  The merged SUM is independent of order.
; ============================================================
(newline)
(display "Select fan-in (merge two producers, sum is order-independent):") (newline)

(define (fan-producer ch xs)
  (define (loop ys)
    (if (null? ys)
        (co-done 'prod)
        (co-send ch (car ys) (lambda () (loop (cdr ys))))))
  (lambda () (loop xs)))

(define (fan-merger a b n)
  (define (loop got total)
    (if (= got n)
        (co-done (list 'sum total))
        (co-select
          (list (alt-recv a (lambda (v) (loop (+ got 1) (+ total v))))
                (alt-recv b (lambda (v) (loop (+ got 1) (+ total v))))))))
  (lambda () (loop 0 0)))

; run it BOTH ways: producers first, and merger first (forces select to park
; and be woken).  Both must give the same total.
(define fa1 (co-channel)) (define fb1 (co-channel))
(define merge-prod-first
  (csp-run (list (fan-producer fa1 (list 1 2 3))
                 (fan-producer fb1 (list 10 20 30))
                 (fan-merger fa1 fb1 6)) 5000))
(define fa2 (co-channel)) (define fb2 (co-channel))
(define merge-merger-first
  (csp-run (list (fan-merger fa2 fb2 6)
                 (fan-producer fa2 (list 1 2 3))
                 (fan-producer fb2 (list 10 20 30))) 5000))

(must "fan-in (producers first) sum = 66"
      (equal? (tagged (run-results merge-prod-first) 'sum) 66))
(must "fan-in (merger first, select parks then wakes) sum = 66"
      (equal? (tagged (run-results merge-merger-first) 'sum) 66))

; ============================================================
; 3. FORK / JOIN: a boss forks N workers; each sends i*i; the boss
;    collects all N and sums them.
; ============================================================
(newline)
(display "Fork/join (boss forks 4 workers i*i, collects the sum):") (newline)
(define jres (co-channel))
(define (jworker i) (lambda () (co-send jres (* i i) (lambda () (co-done 'w)))))
(define (jcollect got total)
  (if (= got 4)
      (co-done (list 'sum total))
      (co-recv jres (lambda (v) (jcollect (+ got 1) (+ total v))))))
(define (jboss)
  (co-fork (jworker 1)
   (lambda () (co-fork (jworker 2)
    (lambda () (co-fork (jworker 3)
     (lambda () (co-fork (jworker 4)
      (lambda () (jcollect 0 0))))))))))
(define fork-result (csp-run (list jboss) 5000))
(must "fork/join completes (status ok)" (equal? (run-status fork-result) 'ok))
(must "sum of 1+4+9+16 = 30" (equal? (tagged (run-results fork-result) 'sum) 30))
(must "boss + 4 workers = 5 results"
      (equal? (length (run-results fork-result)) 5))

; ============================================================
; 4. DEADLOCK DETECTION: two processes each wait to receive on a
;    channel nobody sends to.  The scheduler reports a deadlock with
;    the number of processes still blocked, instead of hanging.
; ============================================================
(newline)
(display "Deadlock detection (two receivers, nobody sends):") (newline)
(define da (co-channel)) (define db (co-channel))
(define (waiter-a) (co-recv da (lambda (v) (co-done v))))
(define (waiter-b) (co-recv db (lambda (v) (co-done v))))
(define dl (csp-run (list waiter-a waiter-b) 5000))
(must "status is 'deadlock" (equal? (run-status dl) 'deadlock))
(must "2 processes reported blocked" (equal? (run-blocked dl) 2))
(must "no process produced a result" (equal? (length (run-results dl)) 0))

(newline)
(display "All cooperative-concurrency checks passed.") (newline)
