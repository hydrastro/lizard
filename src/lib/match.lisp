; -*- lisp -*-
; lib/match.lisp — Pattern matching via cond-based dispatch.
;
; (match-type val
;   ((number?) body ...)
;   ((string?) body ...)
;   ((pair?)   body ...)
;   ((null?)   body ...)
;   (else      body ...))
;
; For now, match-type dispatches on type predicates.
; Full structural pattern matching (destructuring) is planned
; for Phase 3 with syntax-case.

; ---- compose / pipe ----
(define (compose f g)
  (lambda (x) (f (g x))))

(define (pipe . fns)
  (fold-left compose (lambda (x) x) fns))

; ---- when / unless ----
; (These need to be macros for short-circuit, but as functions
; they still work for their common use case.)
(define (when-true test body)
  (if test body '()))

(define (unless-true test body)
  (if test '() body))

; ---- threading helpers ----
(define (-> val . fns)
  (fold-left (lambda (acc f) (f acc)) val fns))

; ---- assoc helpers ----
(define (alist-ref key alist . default)
  (let ((pair (assoc key alist)))
    (if pair
        (cdr pair)
        (if (null? default) #f (car default)))))

(define (alist-set key val alist)
  (cons (cons key val)
        (filter (lambda (pair)
                  (not (equal? (car pair) key)))
                alist)))

; ---- range / enumerate ----
(define (range start end)
  (if (>= start end) '()
    (cons start (range (+ start 1) end))))

(define (enumerate lst)
  (define (go lst i)
    (if (null? lst) '()
      (cons (list i (car lst))
            (go (cdr lst) (+ i 1)))))
  (go lst 0))

; ---- zip ----
(define (zip a b)
  (if (or (null? a) (null? b)) '()
    (cons (list (car a) (car b))
          (zip (cdr a) (cdr b)))))

; ---- flatten ----
(define (flatten lst)
  (if (null? lst) '()
    (if (pair? (car lst))
        (append (flatten (car lst)) (flatten (cdr lst)))
        (cons (car lst) (flatten (cdr lst))))))

; ---- take / drop ----
(define (take n lst)
  (if (or (= n 0) (null? lst)) '()
    (cons (car lst) (take (- n 1) (cdr lst)))))

(define (drop n lst)
  (if (or (= n 0) (null? lst)) lst
    (drop (- n 1) (cdr lst))))

; ---- any / every ----
(define (any pred lst)
  (if (null? lst) #f
    (if (pred (car lst)) #t
      (any pred (cdr lst)))))

(define (every pred lst)
  (if (null? lst) #t
    (if (pred (car lst))
      (every pred (cdr lst))
      #f)))

; ---- partition ----
(define (partition pred lst)
  (list (filter pred lst)
        (filter (lambda (x) (not (pred x))) lst)))

; ---- sort (insertion sort — simple, correct) ----
(define (sort lst less?)
  (define (insert x sorted)
    (if (null? sorted) (list x)
      (if (less? x (car sorted))
          (cons x sorted)
          (cons (car sorted) (insert x (cdr sorted))))))
  (fold-left (lambda (acc x) (insert x acc)) '() lst))

; ---- identity / const / flip ----
(define (identity x) x)
(define (const x) (lambda (_) x))
(define (flip f) (lambda (a b) (f b a)))

; ---- curry / uncurry ----
(define (curry f) (lambda (a) (lambda (b) (f a b))))
(define (uncurry f) (lambda (a b) ((f a) b)))

; ---- repeat ----
(define (repeat n x)
  (if (= n 0) '()
    (cons x (repeat (- n 1) x))))

; ---- string helpers ----
(define (string-repeat s n)
  (string-join (repeat n s) ""))

; ---- juxt (apply multiple fns to same arg) ----
(define (juxt . fns)
  (lambda (x) (map (lambda (f) (f x)) fns)))

; ---- complement ----
(define (complement pred)
  (lambda (x) (not (pred x))))

; ---- memoize (simple, list-based) ----
(define (memoize f)
  (define cache (atom '()))
  (lambda (x)
    (let ((cached (assoc x (deref cache))))
      (if cached
          (cdr cached)
          (let ((result (f x)))
            (swap! cache (lambda (c) (cons (cons x result) c)))
            result)))))

; ---- reduce (alias for fold-left) ----
(define reduce fold-left)

; ---- frequencies: count occurrences ----
(define (frequencies lst)
  (fold-left
    (lambda (acc x)
      (let ((count (alist-ref x acc 0)))
        (alist-set x (+ count 1) acc)))
    '()
    lst))

; ---- group-by: group elements by function ----
(define (group-by f lst)
  (fold-left
    (lambda (acc x)
      (let ((key (f x))
            (existing (alist-ref key acc '())))
        (alist-set key (cons x existing) acc)))
    '()
    lst))

; ---- interleave two lists ----
(define (interleave a b)
  (if (or (null? a) (null? b)) '()
    (cons (car a) (cons (car b) (interleave (cdr a) (cdr b))))))

; ---- scan (prefix sums) ----
(define (scan f init lst)
  (if (null? lst) (list init)
    (cons init (scan f (f init (car lst)) (cdr lst)))))

; ---- iterate: apply f n times ----
(define (iterate f n x)
  (if (= n 0) x (iterate f (- n 1) (f x))))

; ---- unfold: generate list from seed ----
(define (unfold pred f seed)
  (if (pred seed) '()
    (cons (f seed)
          (unfold pred f (f seed)))))

; ---- tabulate: build list from function ----
(define (tabulate f n)
  (map f (range 0 n)))

; ---- sum / product ----
(define (sum lst) (fold-left + 0 lst))
(define (product lst) (fold-left * 1 lst))

; ---- minimum / maximum of list ----
(define (minimum lst)
  (fold-left min (car lst) (cdr lst)))
(define (maximum lst)
  (fold-left max (car lst) (cdr lst)))

; ---- assoc-update ----
(define (assoc-update key f default alist)
  (alist-set key (f (alist-ref key alist default)) alist))

; ---- string-repeat ----
(define (string-repeat s n)
  (string-join (repeat n s) ""))

; ---- pipeline macro (-> already defined) ----
; ---- thread-last (->> val f1 f2 ...) ----
(define (->> val . fns)
  (fold-left (lambda (acc f) (f acc)) val fns))
