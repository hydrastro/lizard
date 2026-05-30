; -*- lisp -*-
; lib/data.lisp — Functional data structures for lizard.
;
; All structures are immutable (persistent). Operations return
; new structures rather than mutating.

; ============================================================
;  STACK — LIFO (last in, first out)
; ============================================================
; Represented as a plain list. Top of stack is the head.

(define (stack-empty) '())
(define (stack-empty? s) (null? s))
(define (stack-push s x) (cons x s))
(define (stack-peek s) (car s))
(define (stack-pop s) (cdr s))
(define (stack-size s) (length s))

; ============================================================
;  QUEUE — FIFO (first in, first out)
; ============================================================
; Banker's queue: two lists (front, back). Amortized O(1).
; A queue is (cons front back). Enqueue to back, dequeue from front.

(define (queue-empty) (cons '() '()))
(define (queue-empty? q)
  (and (null? (car q)) (null? (cdr q))))

(define (queue-enqueue q x)
  (cons (car q) (cons x (cdr q))))

; Normalize: if front is empty, reverse back into front.
(define (queue-normalize q)
  (if (null? (car q))
      (cons (reverse (cdr q)) '())
      q))

(define (queue-peek q)
  (let ((nq (queue-normalize q)))
    (car (car nq))))

(define (queue-dequeue q)
  (let ((nq (queue-normalize q)))
    (cons (cdr (car nq)) (cdr nq))))

; ============================================================
;  SET — unordered collection of unique elements
; ============================================================
; Represented as a list without duplicates.

(define (set-empty) '())
(define (set-empty? s) (null? s))
(define (set-member? s x) (member x s))

(define (set-add s x)
  (if (member x s) s (cons x s)))

(define (set-remove s x)
  (filter (lambda (y) (not (equal? x y))) s))

(define (set-from-list lst)
  (fold-left set-add (set-empty) lst))

(define (set-union a b)
  (fold-left set-add a b))

(define (set-intersect a b)
  (filter (lambda (x) (member x b)) a))

(define (set-difference a b)
  (filter (lambda (x) (not (member x b))) a))

(define (set-size s) (length s))

; ============================================================
;  BINARY SEARCH TREE — ordered map
; ============================================================
; A node is (list key value left right). Empty is '().

(define (bst-empty) '())
(define (bst-empty? t) (null? t))
(define (bst-node key val left right) (list key val left right))
(define (bst-key t) (car t))
(define (bst-val t) (car (cdr t)))
(define (bst-left t) (car (cdr (cdr t))))
(define (bst-right t) (car (cdr (cdr (cdr t)))))

(define (bst-insert t key val)
  (if (bst-empty? t)
      (bst-node key val (bst-empty) (bst-empty))
      (let ((k (bst-key t)))
        (if (< key k)
            (bst-node k (bst-val t)
                      (bst-insert (bst-left t) key val)
                      (bst-right t))
            (if (> key k)
                (bst-node k (bst-val t)
                          (bst-left t)
                          (bst-insert (bst-right t) key val))
                ; key == k: update value
                (bst-node key val (bst-left t) (bst-right t)))))))

(define (bst-lookup t key)
  (if (bst-empty? t)
      #f
      (let ((k (bst-key t)))
        (if (< key k) (bst-lookup (bst-left t) key)
          (if (> key k) (bst-lookup (bst-right t) key)
            (bst-val t))))))

; In-order traversal: returns sorted list of (key value) pairs.
(define (bst-inorder t)
  (if (bst-empty? t) '()
    (append (bst-inorder (bst-left t))
            (cons (list (bst-key t) (bst-val t))
                  (bst-inorder (bst-right t))))))

(define (bst-from-list pairs)
  (fold-left (lambda (t pair) (bst-insert t (car pair) (car (cdr pair))))
             (bst-empty)
             pairs))

; ============================================================
;  PRIORITY QUEUE — via sorted list (simple)
; ============================================================

(define (pqueue-empty) '())
(define (pqueue-empty? q) (null? q))

(define (pqueue-insert q priority val)
  (if (null? q)
      (list (cons priority val))
      (if (< priority (car (car q)))
          (cons (cons priority val) q)
          (cons (car q) (pqueue-insert (cdr q) priority val)))))

(define (pqueue-min q) (cdr (car q)))
(define (pqueue-remove-min q) (cdr q))
