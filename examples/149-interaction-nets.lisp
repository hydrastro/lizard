; 149-interaction-nets.lisp — the syntax GRAPH.
;
; Phase 10 adds an interaction-combinator runtime (the model behind Lafont's
; interaction nets and the HVM / HigherOrderCO machine).  A lambda term is
; compiled to a GRAPH whose nodes are wired port-to-port, with explicit DUP
; (sharing) and ERA (erasure) agents, and reduced by purely local graph
; rewriting.  Sharing makes the reduction Levy-optimal: a duplicated function
; is reduced once, not once per use.
;
;   (inet-normalize '<term>)  reduce on the net, read back an integer (or #f)
;   (inet-cost '<term>)       number of graph rewrites it took (a sharing meter)
;
; Surface grammar:  int | symbol | (lambda (x ...) body) | (op a b) | (f a ...)
; with op in + - * / % = < > .  Two innovations over HVM2: numbers are exact
; GMP integers (not 24-bit), and the cost meter exposes the sharing directly.
;
; Self-checking: each `must` raises on failure (non-zero exit).

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'inet-check-failed)))

; Church numeral n, as a quoted term: (lambda (f) (lambda (x) (f (f ... x))))
(define (church-term n)
  (define (apply-n k inner)
    (if (= k 0) inner (apply-n (- k 1) (list 'f inner))))
  (list 'lambda '(f) (list 'lambda '(x) (apply-n n 'x))))

; church n applied to succ = (lambda (n) (+ n 1)) and base 0
(define (church-eval n)
  (inet-normalize
    (list (list (church-term n) '(lambda (n) (+ n 1))) 0)))

(display "interaction nets: a lambda term reduced as a graph") (newline)
(newline)

(display "Exact arithmetic in the net (bignums, not 24-bit):") (newline)
(must "3 + 4 = 7"                 (= (inet-normalize '(+ 3 4)) 7))
(must "(6*3)+(10-1) = 27"         (= (inet-normalize '(+ (* 6 3) (- 10 1))) 27))
(must "10^12 * 10^12 = 10^24"
      (= (inet-normalize '(* 1000000000000 1000000000000))
         1000000000000000000000000))

(newline)
(display "Beta reduction and erasure:") (newline)
(must "(lambda (x) x) 42 = 42"    (= (inet-normalize '((lambda (x) x) 42)) 42))
(must "K: (K 7 99) = 7"           (= (inet-normalize
                                       '((lambda (a) (lambda (b) a)) 7 99)) 7))
(must "unused arg is erased"      (= (inet-normalize
                                       '((lambda (a) (lambda (b) a)) 5 999)) 5))

(newline)
(display "Sharing: a variable used twice forces a DUP node:") (newline)
(must "(lambda (x) (* x x)) 9 = 81"
      (= (inet-normalize '((lambda (x) (* x x)) 9)) 81))

(newline)
(display "Church numerals over succ — copies `succ` n times via DUP:") (newline)
(must "church 0 = 0"  (= (church-eval 0) 0))
(must "church 1 = 1"  (= (church-eval 1) 1))
(must "church 2 = 2"  (= (church-eval 2) 2))
(must "church 5 = 5"  (= (church-eval 5) 5))
(must "church 10 = 10" (= (church-eval 10) 10))

(newline)
(display "The cost meter shows sharing is LINEAR, not exponential:") (newline)
(define c3  (inet-cost (list (list (church-term 3)  '(lambda (n) (+ n 1))) 0)))
(define c6  (inet-cost (list (list (church-term 6)  '(lambda (n) (+ n 1))) 0)))
(define c12 (inet-cost (list (list (church-term 12) '(lambda (n) (+ n 1))) 0)))
(display "  interactions for church 3 / 6 / 12 : ")
(display c3) (display " / ") (display c6) (display " / ") (display c12) (newline)
; doubling the numeral roughly doubles the cost — linear growth, not blow-up
(must "cost grows linearly (c6 < 3*c3)"   (< c6 (* 3 c3)))
(must "cost grows linearly (c12 < 3*c6)"  (< c12 (* 3 c6)))

(newline)
(display "Non-numeric normal forms read back as #f (integers only in v1):") (newline)
(must "a bare lambda is not an integer" (equal? (inet-normalize '(lambda (x) x)) #f))

(newline)
(display "All interaction-net checks passed.") (newline)
