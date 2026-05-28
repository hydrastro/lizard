; -*- lisp -*-
; ============================================================
;  EXAMPLE 100 — Graph algorithms
; ============================================================

(import "match.lisp")
(import "graph.lisp")

; A directed graph:
;   a → b, c
;   b → d
;   c → d
;   d → e
;   e → (nothing)
(define g '((a b c) (b d) (c d) (d e) (e)))

(display "=== GRAPH STRUCTURE ===") (newline)
(display "  nodes: ") (display (graph-nodes g)) (newline)
(display "  neighbors of a: ") (display (graph-neighbors g 'a)) (newline)
(display "  edge count: ") (display (graph-edge-count g)) (newline)
(display "  a→b edge? ") (display (if (graph-has-edge? g 'a 'b) "yes" "no")) (newline)
(display "  a→d edge? ") (display (if (graph-has-edge? g 'a 'd) "yes" "no")) (newline)
(newline)

(display "=== TRAVERSALS ===") (newline)
(display "  DFS from a: ") (display (graph-dfs g 'a)) (newline)
(display "  BFS from a: ") (display (graph-bfs g 'a)) (newline)
(newline)

(display "=== REACHABILITY ===") (newline)
(display "  a reaches e? ") (display (graph-reachable? g 'a 'e)) (newline)
(display "  e reaches a? ") (display (graph-reachable? g 'e 'a)) (newline)
(newline)

(display "=== PATH FINDING ===") (newline)
(display "  path a→e: ") (display (graph-find-path g 'a 'e)) (newline)
(display "  path e→a: ") (display (graph-find-path g 'e 'a)) (newline)
(newline)

(display "=== TOPOLOGICAL SORT ===") (newline)
(display "  topo order: ") (display (graph-topo-sort g)) (newline)
(display "  (dependencies come before dependents)") (newline)
(newline)

(display "=== DEGREES ===") (newline)
(display "  out-degree of a: ") (display (graph-out-degree g 'a)) (newline)
(display "  in-degree of d: ") (display (graph-in-degree g 'd)) (newline)
(newline)

(display "=== BUILDING A GRAPH ===") (newline)
(define g2 (graph-add-edge (graph-add-edge '() 'x 'y) 'y 'z))
(display "  built x→y→z: ") (display g2) (newline)
(display "  DFS from x: ") (display (graph-dfs g2 'x)) (newline)
(newline)

(display "=== DEPENDENCY GRAPH EXAMPLE ===") (newline)
; Module dependencies: app depends on db and ui; db depends on core; etc.
(define deps '((app db ui) (db core) (ui core) (core)))
(display "  build order: ") (display (graph-topo-sort deps)) (newline)
(display "  (core builds first, app builds last)") (newline)
(newline)

(display "=== End of graph algorithms ===") (newline)
