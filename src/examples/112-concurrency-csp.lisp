; -*- lisp -*-
; ============================================================
;  EXAMPLE 112 — Track C: cooperative concurrency
; ============================================================

(import "match.lisp")
(import "csp.lisp")

(display "=== CHANNELS (message passing) ===") (newline)
(define ch (make-channel))
(chan-send! ch 'hello)
(chan-send! ch 'world)
(chan-send! ch 42)
(display "  channel size: ") (display (chan-size ch)) (newline)
(display "  recv: ") (display (chan-recv! ch)) (newline)
(display "  recv: ") (display (chan-recv! ch)) (newline)
(display "  recv: ") (display (chan-recv! ch)) (newline)
(display "  recv (empty): ") (display (chan-recv! ch)) (newline)
(newline)

(display "=== COOPERATIVE SCHEDULER ===") (newline)
; Two tasks that each print a few times, interleaved by the scheduler.
(define log (make-channel))
(define sched (make-scheduler))
(spawn! sched (repeat-task 3 (lambda (i) (chan-send! log (list 'A i)))))
(spawn! sched (repeat-task 3 (lambda (i) (chan-send! log (list 'B i)))))
(define steps (run-scheduler! sched))
(display "  scheduler ran ") (display steps) (display " steps") (newline)
(display "  interleaved log: ") (display (deref log)) (newline)
(display "  (A and B tasks alternate — cooperative interleaving)") (newline)
(newline)

(display "=== FUTURES (compute once) ===") (newline)
(define compute-count (atom 0))
(define fut
  (make-future (lambda ()
                 (swap! compute-count (lambda (n) (+ n 1)))
                 (* 6 7))))
(display "  forced? ") (display (future-forced? fut)) (newline)
(display "  force: ") (display (force-future fut)) (newline)
(display "  force again: ") (display (force-future fut)) (newline)
(display "  times computed (should be 1): ")
(display (deref compute-count)) (newline)
(define doubled (future-map (lambda (x) (* x 2)) fut))
(display "  future-map *2: ") (display (force-future doubled)) (newline)
(newline)

(display "=== AGENTS (actor-style state) ===") (newline)
; A bank-account agent: messages (deposit n) / (withdraw n).
(define account
  (make-agent 100
    (lambda (balance msg)
      (if (equal? (car msg) 'deposit)
          (+ balance (car (cdr msg)))
          (if (equal? (car msg) 'withdraw)
              (- balance (car (cdr msg)))
              balance)))))
(display "  initial balance: ") (display (agent-state account)) (newline)
(agent-send! account '(deposit 50))
(display "  after deposit 50: ") (display (agent-state account)) (newline)
(agent-send-all! account '((withdraw 30) (deposit 10) (withdraw 5)))
(display "  after -30 +10 -5: ") (display (agent-state account)) (newline)
(newline)

(display "=== COUNTER AGENT ===") (newline)
(define counter-agent
  (make-agent 0 (lambda (n msg)
                  (if (equal? msg 'inc) (+ n 1)
                    (if (equal? msg 'dec) (- n 1) n)))))
(agent-send-all! counter-agent '(inc inc inc dec inc))
(display "  counter after inc×4 dec×1: ")
(display (agent-state counter-agent)) (newline)
(newline)

(display "=== Track C milestone: concurrency model ✓ ===") (newline)
