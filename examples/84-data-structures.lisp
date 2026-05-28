; -*- lisp -*-
; ============================================================
;  EXAMPLE 84 — Functional data structures
; ============================================================

(import "data.lisp")

(display "=== STACK (LIFO) ===") (newline)
(define s (stack-push (stack-push (stack-push (stack-empty) 1) 2) 3))
(display "  Push 1,2,3. Top: ") (display (stack-peek s)) (newline)
(display "  Size: ") (display (stack-size s)) (newline)
(display "  After pop, top: ") (display (stack-peek (stack-pop s))) (newline)
(newline)

(display "=== QUEUE (FIFO) ===") (newline)
(define q (queue-enqueue (queue-enqueue (queue-enqueue (queue-empty) 'a) 'b) 'c))
(display "  Enqueue a,b,c. Front: ") (display (queue-peek q)) (newline)
(define q2 (queue-dequeue q))
(display "  After dequeue, front: ") (display (queue-peek q2)) (newline)
(newline)

(display "=== SET ===") (newline)
(define s1 (set-from-list '(1 2 3 4 5)))
(define s2 (set-from-list '(3 4 5 6 7)))
(display "  s1 = {1,2,3,4,5}, s2 = {3,4,5,6,7}") (newline)
(display "  union: ") (display (set-union s1 s2)) (newline)
(display "  intersection: ") (display (set-intersect s1 s2)) (newline)
(display "  difference (s1-s2): ") (display (set-difference s1 s2)) (newline)
(display "  member? 3: ") (display (if (set-member? s1 3) "yes" "no")) (newline)
(display "  member? 9: ") (display (if (set-member? s1 9) "yes" "no")) (newline)
(newline)

(display "=== BINARY SEARCH TREE ===") (newline)
(define tree (bst-from-list '((5 "five") (3 "three") (8 "eight")
                              (1 "one") (4 "four") (7 "seven"))))
(display "  Inserted 6 key-value pairs.") (newline)
(display "  lookup 4: ") (display (bst-lookup tree 4)) (newline)
(display "  lookup 8: ") (display (bst-lookup tree 8)) (newline)
(display "  lookup 99: ") (display (bst-lookup tree 99)) (newline)
(display "  in-order (sorted): ") (newline)
(display "    ") (display (bst-inorder tree)) (newline)
(newline)

(display "=== PRIORITY QUEUE ===") (newline)
(define pq (pqueue-insert
             (pqueue-insert
               (pqueue-insert (pqueue-empty) 3 'medium)
               1 'high)
             5 'low))
(display "  Inserted (3,medium) (1,high) (5,low)") (newline)
(display "  min priority item: ") (display (pqueue-min pq)) (newline)
(display "  next min: ") (display (pqueue-min (pqueue-remove-min pq))) (newline)
(newline)

(display "=== End of data structures ===") (newline)
