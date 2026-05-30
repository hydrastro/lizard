; -*- lisp -*-
; lib/coroutine.lisp — CSP coroutines with blocking (Track C → 100%).
;
; True blocking communication without host continuations: coroutines
; are step-functions that return an EFFECT describing what to do next.
; The scheduler interprets effects, PARKING a coroutine that blocks on
; an empty receive and WAKING it when a matching send arrives. This is
; the CSP model: processes synchronize through channels.
;
; A coroutine is a thunk  () -> step.  A step is one of:
;   (co-done v)         finished with result v
;   (co-yield v k)      emit v; resume with thunk k
;   (co-send ch v k)    send v on ch; resume with thunk k
;   (co-recv ch k)      receive from ch; resume with (k value)

(import "match.lisp")

; ============================================================
;  STEP CONSTRUCTORS
; ============================================================

(define (co-done v) (list 'done v))
(define (co-yield v k) (list 'yield v k))
(define (co-send ch v k) (list 'send ch v k))
(define (co-recv ch k) (list 'recv ch k))

(define (step-tag s) (car s))

; ============================================================
;  CHANNELS  (buffer + parked receivers)
; ============================================================
; A channel: (list 'cchan buffer-atom waiters-atom)
;   buffer  : values sent but not yet received (FIFO)
;   waiters : receiver continuations parked on an empty channel

(define (co-channel) (list 'cchan (atom '()) (atom '())))

(define (chan-buffer ch) (car (cdr ch)))
(define (chan-waiters ch) (car (cdr (cdr ch))))

(define (chan-push-value! ch v)
  (swap! (chan-buffer ch) (lambda (q) (append q (list v)))))

(define (chan-pop-value! ch)
  (let ((q (deref (chan-buffer ch))))
    (reset! (chan-buffer ch) (cdr q))
    (car q)))

(define (chan-has-value? ch) (not (null? (deref (chan-buffer ch)))))

(define (chan-park-waiter! ch receiver)
  (swap! (chan-waiters ch) (lambda (w) (append w (list receiver)))))

(define (chan-has-waiter? ch) (not (null? (deref (chan-waiters ch)))))

(define (chan-pop-waiter! ch)
  (let ((w (deref (chan-waiters ch))))
    (reset! (chan-waiters ch) (cdr w))
    (car w)))

; ============================================================
;  SCHEDULER
; ============================================================
; ready : a queue (atom) of resumable coroutine thunks.
; Runs round-robin, parking blocked receivers, until the queue
; empties. Collects 'done results and 'yield-ed values.

(define (make-corun)
  (list 'corun (atom '()) (atom '()) (atom '())))
; fields: ready-queue, results, yields

(define (corun-ready r) (car (cdr r)))
(define (corun-results r) (car (cdr (cdr r))))
(define (corun-yields r) (car (cdr (cdr (cdr r)))))

(define (co-spawn! r thunk)
  (swap! (corun-ready r) (lambda (q) (append q (list thunk))))
  r)

(define (ready-empty? r) (null? (deref (corun-ready r))))

(define (ready-pop! r)
  (let ((q (deref (corun-ready r))))
    (reset! (corun-ready r) (cdr q))
    (car q)))

(define (record-result! r v)
  (swap! (corun-results r) (lambda (rs) (append rs (list v)))))

(define (record-yield! r v)
  (swap! (corun-yields r) (lambda (ys) (append ys (list v)))))

; Run all coroutines to completion (bounded steps for safety).
(define (co-run! r max-steps)
  (define (loop steps)
    (if (= steps max-steps) steps
      (if (ready-empty? r) steps
        (let ((thunk (ready-pop! r)))
          (let ((s (thunk)))
            (dispatch r s)
            (loop (+ steps 1)))))))
  (loop 0))

(define (dispatch r s)
  (let ((tag (step-tag s)))
    (if (equal? tag 'done)
        (record-result! r (car (cdr s)))
      (if (equal? tag 'yield)
          (begin (record-yield! r (car (cdr s)))
                 (co-spawn! r (car (cdr (cdr s)))))
        (if (equal? tag 'send)
            (dispatch-send r s)
          (if (equal? tag 'recv)
              (dispatch-recv r s)
              #f))))))

(define (dispatch-send r s)
  (let ((ch (car (cdr s)))
        (v (car (cdr (cdr s))))
        (k (car (cdr (cdr (cdr s))))))
    ; if a receiver is parked, hand it the value directly; else buffer.
    (if (chan-has-waiter? ch)
        (let ((receiver (chan-pop-waiter! ch)))
          (co-spawn! r (lambda () (receiver v))))
        (chan-push-value! ch v))
    (co-spawn! r k)))           ; sender continues immediately

(define (dispatch-recv r s)
  (let ((ch (car (cdr s)))
        (k (car (cdr (cdr s)))))
    (if (chan-has-value? ch)
        ; value ready: resume now with it
        (let ((v (chan-pop-value! ch)))
          (co-spawn! r (lambda () (k v))))
        ; empty: PARK this coroutine as a waiter (do not requeue)
        (chan-park-waiter! ch k))))

; ============================================================
;  CONVENIENCE: build common coroutines
; ============================================================

; Producer: send 0..n-1 on ch, then finish.
(define (co-producer ch n)
  (define (go i)
    (if (>= i n)
        (co-done 'produced)
        (co-send ch i (lambda () (go (+ i 1))))))
  (lambda () (go 0)))

; Consumer: receive n values, accumulate, then finish.
(define (co-consumer ch n)
  (define (go i acc)
    (if (>= i n)
        (co-done (reverse acc))
        (co-recv ch (lambda (v) (go (+ i 1) (cons v acc))))))
  (lambda () (go 0 '())))

; Transformer: receive n values, send (f v) on out, then finish.
(define (co-transform in out n f)
  (define (go i)
    (if (>= i n)
        (co-done 'transformed)
        (co-recv in (lambda (v)
                      (co-send out (f v) (lambda () (go (+ i 1))))))))
  (lambda () (go 0)))
