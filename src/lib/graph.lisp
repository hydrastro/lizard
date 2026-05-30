; -*- lisp -*-
; lib/graph.lisp — Graph algorithms.
;
; A graph is an adjacency list: an alist mapping each node to a
; list of its neighbors.
;   (define g '((a b c) (b d) (c d) (d)))
; means a→b, a→c, b→d, c→d, d→(nothing).

(import "match.lisp")    ; for set-style helpers via member

; ---- accessors ----

(define (graph-nodes g)
  (map car g))

(define (graph-neighbors g node)
  (let ((entry (assoc node g)))
    (if entry (cdr entry) '())))

(define (graph-has-edge? g from to)
  (member to (graph-neighbors g from)))

(define (graph-edge-count g)
  (fold-left (lambda (acc entry) (+ acc (length (cdr entry)))) 0 g))

; ---- construction ----

(define (graph-add-edge g from to)
  (let ((entry (assoc from g)))
    (if entry
        (cons (cons from (cons to (cdr entry)))
              (filter (lambda (e) (not (equal? (car e) from))) g))
        (cons (list from to) g))))

; ---- depth-first search ----
; Returns the list of nodes reachable from start, in DFS order.

(define (graph-dfs g start)
  (define (go stack visited)
    (if (null? stack)
        (reverse visited)
        (let ((node (car stack)))
          (if (member node visited)
              (go (cdr stack) visited)
              (go (append (graph-neighbors g node) (cdr stack))
                  (cons node visited))))))
  (go (list start) '()))

; ---- breadth-first search ----
; Returns nodes reachable from start, in BFS order.

(define (graph-bfs g start)
  (define (go queue visited)
    (if (null? queue)
        (reverse visited)
        (let ((node (car queue)))
          (if (member node visited)
              (go (cdr queue) visited)
              (go (append (cdr queue) (graph-neighbors g node))
                  (cons node visited))))))
  (go (list start) '()))

; ---- reachability ----

(define (graph-reachable? g from to)
  (if (member to (graph-dfs g from)) #t #f))

; ---- topological sort (for DAGs) ----
; Returns nodes in dependency order (a node appears after all the
; nodes that point to it). Uses DFS post-order.

(define (graph-topo-sort g)
  (define visited (atom '()))
  (define result (atom '()))
  (define (visit node)
    (if (member node (deref visited))
        '()
        (begin
          (swap! visited (lambda (v) (cons node v)))
          (for-each visit (graph-neighbors g node))
          (swap! result (lambda (r) (cons node r))))))
  (for-each visit (graph-nodes g))
  (deref result))

; ---- connected component (reachable set from a node) ----

(define (graph-component g start)
  (graph-dfs g start))

; ---- path finding (returns a path as a list, or #f) ----

(define (graph-find-path g from to)
  (define (go node target visited)
    (if (equal? node target)
        (list node)
        (if (member node visited)
            #f
            (let ((sub (try-neighbors (graph-neighbors g node)
                                      target
                                      (cons node visited))))
              (if sub (cons node sub) #f)))))
  (define (try-neighbors neighbors target visited)
    (if (null? neighbors)
        #f
        (let ((path (go (car neighbors) target visited)))
          (if path path
            (try-neighbors (cdr neighbors) target visited)))))
  (go from to '()))

; ---- degree ----

(define (graph-out-degree g node)
  (length (graph-neighbors g node)))

(define (graph-in-degree g node)
  (fold-left
    (lambda (acc entry)
      (if (member node (cdr entry)) (+ acc 1) acc))
    0
    g))
