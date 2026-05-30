; -*- lisp -*-
; lib/csp.lisp — Cooperative concurrency (Track C).
;
; lizard runs on one thread, so concurrency here is COOPERATIVE:
; tasks voluntarily yield by returning a continuation. Built on
; atoms (the only shared mutable state). Provides a scheduler,
; channels (CSP-style message passing), futures, and agents.

(import "match.lisp")

; ============================================================
;  CHANNELS — buffered message queues
; ============================================================
; A channel is an atom holding a list (FIFO: oldest at head).

(define (make-channel) (atom '()))

(define (chan-send! ch msg)
  (swap! ch (lambda (q) (append q (list msg))))
  msg)

(define (chan-empty? ch) (null? (deref ch)))

; Receive: returns (cons 'ok msg) or 'empty (non-blocking).
(define (chan-recv! ch)
  (let ((q (deref ch)))
    (if (null? q)
        'empty
        (begin (reset! ch (cdr q))
               (cons 'ok (car q))))))

(define (chan-peek ch)
  (let ((q (deref ch)))
    (if (null? q) 'empty (cons 'ok (car q)))))

(define (chan-size ch) (length (deref ch)))

; ============================================================
;  SCHEDULER — round-robin cooperative tasks
; ============================================================
; A task step is a thunk returning either:
;   'done                         the task is finished
;   (cons 'next next-thunk)       yield; resume with next-thunk
;
; The scheduler runs steps round-robin until all tasks finish.

(define (make-scheduler) (atom '()))

(define (spawn! sched task)
  (swap! sched (lambda (q) (append q (list task))))
  sched)

(define (task-done? r) (equal? r 'done))
(define (task-next? r) (and (pair? r) (equal? (car r) 'next)))
(define (task-cont r) (cdr r))

; Run the scheduler to completion; returns total steps executed.
(define (run-scheduler! sched)
  (define (loop steps)
    (let ((q (deref sched)))
      (if (null? q) steps
        (let ((task (car q)))
          (reset! sched (cdr q))
          (let ((r (task)))
            (if (task-next? r)
                (begin (spawn! sched (task-cont r)) (loop (+ steps 1)))
                (loop (+ steps 1))))))))
  (loop 0))

; Helper: build a counting task that yields n times, calling
; (body i) on each step.
(define (repeat-task n body)
  (define (step i)
    (lambda ()
      (if (>= i n) 'done
        (begin (body i)
               (cons 'next (step (+ i 1)))))))
  (step 0))

; ============================================================
;  FUTURES / PROMISES — compute-once values
; ============================================================
; A future wraps a thunk. The first `force` runs it and caches.

(define (make-future thunk)
  (atom (cons 'pending thunk)))

(define (future-forced? fut)
  (equal? (car (deref fut)) 'ready))

(define (force-future fut)
  (let ((state (deref fut)))
    (if (equal? (car state) 'ready)
        (cdr state)
        (let ((value ((cdr state))))
          (reset! fut (cons 'ready value))
          value))))

; Map a function over a future, producing a new future.
(define (future-map f fut)
  (make-future (lambda () (f (force-future fut)))))

; ============================================================
;  AGENTS — state + message handler (actor-style)
; ============================================================
; An agent holds state in an atom and a handler (state msg) -> state.
; Messages are applied synchronously in this cooperative model.

(define (make-agent init handler)
  (list 'agent (atom init) handler))

(define (agent? x) (and (pair? x) (equal? (car x) 'agent)))
(define (agent-cell a) (car (cdr a)))
(define (agent-handler a) (car (cdr (cdr a))))

(define (agent-send! a msg)
  (let ((cell (agent-cell a))
        (h (agent-handler a)))
    (swap! cell (lambda (state) (h state msg)))
    a))

(define (agent-state a) (deref (agent-cell a)))

; Send a sequence of messages to an agent.
(define (agent-send-all! a msgs)
  (for-each (lambda (m) (agent-send! a m)) msgs)
  a)
