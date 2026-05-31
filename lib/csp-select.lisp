; -*- lisp -*-
; lib/csp-select.lisp — completing cooperative concurrency: select, fork,
;                       deadlock detection — on COPY-SAFE channels.
;
; lizard runs on one thread (the C89 / -pedantic-errors build admits no
; portable OS threads) and call/cc is escape-only, so the only SOUND
; concurrency model is COOPERATIVE single-threaded scheduling.  Processes are
; step-functions returning an EFFECT the scheduler interprets — (co-done v),
; (co-yield v k), (co-send ch v k), (co-recv ch k) — exactly as in
; lib/coroutine.lisp, whose step constructors we reuse.
;
; IMPORTANT representation choice: the interpreter copies compound values when
; they are placed inside other structures, so a mutable cell (atom) carried
; INSIDE an effect step would be duplicated and the two ends would stop sharing
; state.  Channels here are therefore plain INTEGER handles into one shared
; REGISTRY atom; integers copy harmlessly, and every channel operation reaches
; the single registry directly.  This keeps send/receive correct regardless of
; scheduling order (buffered send, then later receive, still rendezvous).
;
; New pieces on top of the core:
;
;   (co-select alts)   the defining CSP/Go primitive — a non-deterministic
;                      CHOICE over channel ops.  Commits to the first READY
;                      alternative; if none is ready it parks a cancellable
;                      waiter on every receive channel, sharing one commit flag,
;                      and the first send to any of them wins (losers give the
;                      value back and retire).  Alt: (alt-recv ch k) | (alt-send ch v k).
;   (co-fork child k)  structured spawning: launch `child`, continue `k`.
;   deadlock detection the scheduler counts LIVE (spawned-but-unfinished)
;                      processes, so a drained ready queue with live > 0 is a
;                      DEADLOCK, distinct from normal completion.
;
; csp-run returns one of:
;   (list 'ok results) | (list 'deadlock results live) | (list 'timeout results)

(import "coroutine.lisp")

; ============================================================
;  EXTRA EFFECTS
; ============================================================

(define (co-select alts) (list 'select alts))
(define (co-fork child k) (list 'fork child k))
(define (co-noop) (list 'noop))

(define (alt-recv ch k)   (list 'arecv ch k))
(define (alt-send ch v k) (list 'asend ch v k))
(define (alt-kind a) (car a))
(define (alt-ch a)   (car (cdr a)))
(define (alt-recv-k a) (car (cdr (cdr a))))
(define (alt-send-v a) (car (cdr (cdr a))))
(define (alt-send-k a) (car (cdr (cdr (cdr a)))))

; ============================================================
;  CHANNELS  — integer handles into one shared registry  (copy-safe)
; ============================================================
; *chan-reg* : atom holding an alist  id -> (list id buffer waiters)
;   buffer  : FIFO list of values sent but not yet received
;   waiters : FIFO list of receiver continuations parked on an empty channel
; A channel value is just its integer id, so carrying it inside an effect
; step (which the interpreter may copy) never severs the shared state.

(define *chan-reg* (atom '()))
(define *chan-n* (atom 0))

(define (co-channel)
  (let ((id (deref *chan-n*)))
    (reset! *chan-n* (+ id 1))
    (swap! *chan-reg* (lambda (a) (cons (list id '() '()) a)))
    id))

(define (chan-entry id)
  (define (go a)
    (if (null? a) #f
      (if (equal? (car (car a)) id) (car a)
        (go (cdr a)))))
  (go (deref *chan-reg*)))

(define (chan-buffer id)  (car (cdr (chan-entry id))))
(define (chan-waiters id) (car (cdr (cdr (chan-entry id)))))

(define (chan-set! id buf wait)
  (swap! *chan-reg*
    (lambda (a)
      (map (lambda (e) (if (equal? (car e) id) (list id buf wait) e)) a))))

(define (chan-push-value! id v)
  (chan-set! id (append (chan-buffer id) (list v)) (chan-waiters id)))

(define (chan-has-value? id) (not (null? (chan-buffer id))))

(define (chan-pop-value! id)
  (let ((b (chan-buffer id)))
    (chan-set! id (cdr b) (chan-waiters id))
    (car b)))

; A parked waiter is (list 'w sid fn): `sid` is the integer id of the receive
; or select it belongs to, and `fn` is the 1-arg receiver.  A select registers
; one waiter per channel sharing ONE sid; claiming any of them records the sid
; as committed, so the siblings instantly go inactive.  Crucially `sid` is an
; integer (copy-safe) and the committed set lives in a single top-level atom —
; never inside the per-channel registry, which the interpreter would copy.
(define *committed* (atom '()))
(define *sel-n* (atom 0))
(define (fresh-sel-id)
  (let ((id (deref *sel-n*))) (reset! *sel-n* (+ id 1)) id))
(define (sel-committed? sid)
  (not (null? (filter (lambda (x) (equal? x sid)) (deref *committed*)))))
(define (commit-sel! sid)
  (swap! *committed* (lambda (c) (cons sid c))))

(define (w-sid w) (car (cdr w)))
(define (w-fn w)  (car (cdr (cdr w))))
(define (w-active? w) (not (sel-committed? (w-sid w))))

(define (chan-park-waiter! id sid fn)
  (chan-set! id (chan-buffer id)
             (append (chan-waiters id) (list (list 'w sid fn)))))

(define (any-active? ws)
  (if (null? ws) #f
    (if (w-active? (car ws)) #t (any-active? (cdr ws)))))

(define (chan-has-waiter? id) (any-active? (chan-waiters id)))

; claim and remove the first ACTIVE waiter (pruning dead ones); record its sid
; as committed so sibling select-waiters on other channels go dead.  -> fn | #f
(define (chan-take-waiter! id)
  (let ((live (filter w-active? (chan-waiters id))))
    (if (null? live) #f
      (begin
        (commit-sel! (w-sid (car live)))
        (chan-set! id (chan-buffer id) (cdr live))
        (w-fn (car live))))))

; ============================================================
;  SCHEDULER  (ready queue + results + LIVE counter + yields)
; ============================================================
; (list 'sched ready-atom results-atom live-atom yields-atom)

(define (make-sched) (list 'sched (atom '()) (atom '()) (atom 0) (atom '())))
(define (sched-ready s)   (car (cdr s)))
(define (sched-results s) (car (cdr (cdr s))))
(define (sched-live s)    (car (cdr (cdr (cdr s)))))
(define (sched-yields s)  (car (cdr (cdr (cdr (cdr s))))))

(define (sched-enqueue! s thunk)
  (swap! (sched-ready s) (lambda (q) (append q (list thunk)))))

(define (sched-spawn! s thunk)            ; a BRAND-NEW fiber: bump live
  (swap! (sched-live s) (lambda (n) (+ n 1)))
  (sched-enqueue! s thunk))

(define (sched-ready-empty? s) (null? (deref (sched-ready s))))

(define (sched-pop! s)
  (let ((q (deref (sched-ready s))))
    (reset! (sched-ready s) (cdr q))
    (car q)))

(define (sched-record! s v)
  (swap! (sched-results s) (lambda (rs) (append rs (list v)))))

(define (sched-yield! s v)
  (swap! (sched-yields s) (lambda (ys) (append ys (list v)))))

(define (sched-done! s v)                 ; a fiber finished: drop live, record
  (swap! (sched-live s) (lambda (n) (- n 1)))
  (sched-record! s v))

; ============================================================
;  SEND / RECV
; ============================================================

(define (csp-send s step)
  (let ((ch (car (cdr step)))
        (v (car (cdr (cdr step))))
        (k (car (cdr (cdr (cdr step))))))
    (let ((fn (chan-take-waiter! ch)))
      (if fn
          (sched-enqueue! s (lambda () (fn v)))
          (chan-push-value! ch v)))
    (sched-enqueue! s k)))                 ; sender continues (same fiber)

(define (csp-recv s step)
  (let ((ch (car (cdr step)))
        (k (car (cdr (cdr step)))))
    (if (chan-has-value? ch)
        (let ((v (chan-pop-value! ch)))
          (sched-enqueue! s (lambda () (k v))))
        (chan-park-waiter! ch (fresh-sel-id) k))))  ; park (own sid): live unchanged

; ============================================================
;  SELECT
; ============================================================

(define (alt-ready? a)
  (if (equal? (alt-kind a) 'asend)
      #t
      (chan-has-value? (alt-ch a))))

(define (find-ready alts)
  (if (null? alts) #f
    (if (alt-ready? (car alts)) (car alts)
      (find-ready (cdr alts)))))

(define (commit-alt! s a)
  (let ((ch (alt-ch a)))
    (if (equal? (alt-kind a) 'asend)
        (begin
          (let ((fn (chan-take-waiter! ch)))
            (if fn
                (sched-enqueue! s (lambda () (fn (alt-send-v a))))
                (chan-push-value! ch (alt-send-v a))))
          (sched-enqueue! s (alt-send-k a)))
        (let ((v (chan-pop-value! ch)))
          (sched-enqueue! s (lambda () ((alt-recv-k a) v)))))))

(define (csp-reg-one s sid a)
  (let ((k (alt-recv-k a)))
    (chan-park-waiter! (alt-ch a) sid (lambda (v) (k v)))))

(define (csp-park-select s alts)
  (let ((sid (fresh-sel-id)))
    (for-each (lambda (a) (csp-reg-one s sid a)) alts)))

(define (csp-select-do s alts)
  (let ((ready (find-ready alts)))
    (if ready
        (commit-alt! s ready)
        (csp-park-select s alts))))

; ============================================================
;  DISPATCH  +  RUN LOOP
; ============================================================

(define (csp-fork s step)
  (let ((child (car (cdr step)))
        (k (car (cdr (cdr step)))))
    (sched-spawn! s child)
    (sched-enqueue! s k)))

(define (csp-dispatch s step)
  (let ((tag (step-tag step)))
    (cond
      ((equal? tag 'done)   (sched-done! s (car (cdr step))))
      ((equal? tag 'noop)   #t)
      ((equal? tag 'yield)  (begin (sched-yield! s (car (cdr step)))
                                   (sched-enqueue! s (car (cdr (cdr step))))))
      ((equal? tag 'send)   (csp-send s step))
      ((equal? tag 'recv)   (csp-recv s step))
      ((equal? tag 'select) (csp-select-do s (car (cdr step))))
      ((equal? tag 'fork)   (csp-fork s step))
      (else #f))))

(define (csp-loop s steps max-steps)
  (if (= steps max-steps)
      (list 'timeout (deref (sched-results s)))
    (if (sched-ready-empty? s)
        (if (> (deref (sched-live s)) 0)
            (list 'deadlock (deref (sched-results s)) (deref (sched-live s)))
            (list 'ok (deref (sched-results s))))
      (let ((thunk (sched-pop! s)))
        (let ((step (thunk)))
          (csp-dispatch s step)
          (csp-loop s (+ steps 1) max-steps))))))

(define (csp-run fibers max-steps)
  (let ((s (make-sched)))
    (for-each (lambda (f) (sched-spawn! s f)) fibers)
    (csp-loop s 0 max-steps)))

(define (run-status r)  (car r))
(define (run-results r) (car (cdr r)))
(define (run-blocked r) (car (cdr (cdr r))))
