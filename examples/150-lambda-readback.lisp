; 150-lambda-readback.lisp — reading normal forms back as lambda terms.
;
; Phase 10's interaction net reduces a lambda term to a normal-form GRAPH.
; `inet-normalize` only observed a number at the root.  `inet-reduce` performs
; full READBACK: after reduction it rebuilds the normal form as a lambda term
; written in de Bruijn notation —
;
;     #k         a bound variable, k binders out (0 = innermost)
;     (lam b)    an abstraction
;     (f a)      an application
;     42         an integer literal
;
; So the graph machine becomes a real lambda evaluator whose output you can
; read.  The hard part is that the reducer's wiring is one-directional (a use
; cannot find its binder); readback first builds a bidirectional snapshot of
; the net, then walks it, following sharing (DUP) chains so that every use of
; a duplicated variable comes back as the same de Bruijn index.
;
; Self-checking: each `must` raises on failure (non-zero exit).

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'readback-check-failed)))

; --- quoted-term builders -------------------------------------------------
(define (church-term n)                 ; (lambda (f) (lambda (x) (f .. x)))
  (define (apply-n k inner)
    (if (= k 0) inner (apply-n (- k 1) (list 'f inner))))
  (list 'lambda '(f) (list 'lambda '(x) (apply-n n 'x))))

(define I '(lambda (x) x))
(define K '(lambda (a) (lambda (b) a)))
(define S '(lambda (f) (lambda (g) (lambda (x) ((f x) (g x))))))
(define PLUS
  '(lambda (m) (lambda (n) (lambda (f) (lambda (x) ((m f) ((n f) x)))))))
(define MUL '(lambda (m) (lambda (n) (lambda (f) (m (n f))))))
(define succ '(lambda (n) (+ n 1)))

(display "reading interaction-net normal forms back as lambda terms")
(newline) (newline)

(display "Abstractions and bound variables (de Bruijn):") (newline)
(must "identity        -> (lam #0)"
      (equal? (inet-reduce I) "(lam #0)"))
(must "K = lx.ly.x     -> (lam (lam #1))"
      (equal? (inet-reduce K) "(lam (lam #1))"))
(must "Church 2        -> (lam (lam (#1 (#1 #0))))"
      (equal? (inet-reduce (church-term 2)) "(lam (lam (#1 (#1 #0))))"))
(must "Church 3        -> (lam (lam (#1 (#1 (#1 #0)))))"
      (equal? (inet-reduce (church-term 3))
              "(lam (lam (#1 (#1 (#1 #0)))))"))

(newline)
(display "Reduction happens on the GRAPH, then we read the result:") (newline)
(must "(I I)           -> (lam #0)"
      (equal? (inet-reduce (list I I)) "(lam #0)"))
(must "S K K           -> (lam #0)            ; SKK = I, by reduction"
      (equal? (inet-reduce (list (list S K) K)) "(lam #0)"))
(must "PLUS 2 3        -> Church 5"
      (equal? (inet-reduce (list (list PLUS (church-term 2)) (church-term 3)))
              "(lam (lam (#1 (#1 (#1 (#1 (#1 #0)))))))"))

(newline)
(display "Numeric normal forms read back as decimals:") (newline)
(must "3 + 4           -> 7"   (equal? (inet-reduce '(+ 3 4)) "7"))
(must "Church 2 succ 0 -> 2"   (equal? (inet-reduce (list (list (church-term 2) succ) 0)) "2"))

(newline)
(display "An HONEST boundary — residual sharing of a COMPOUND subterm:") (newline)
; A bare Church multiplication keeps DUP nodes that are not in active-pair
; position; faithfully expanding that sharing into a tree needs the labelled
; bookkeeping of optimal reduction, which the reader does not do — so it
; REFUSES (returns #f) rather than print a wrong term.
(must "MUL 2 3 (bare)  -> #f  (refuses, does not guess)"
      (equal? (inet-reduce (list (list MUL (church-term 2)) (church-term 3))) #f))
(must "(2 2)   (bare)  -> #f  (same shape)"
      (equal? (inet-reduce (list (church-term 2) (church-term 2))) #f))
; ...yet the very same multiplication reduces to the right number on demand:
(must "MUL 2 3 succ 0  = 6  (value is correct when demanded)"
      (= (inet-normalize
           (list (list (list (list MUL (church-term 2)) (church-term 3)) succ) 0))
         6))

(newline)
(display "All lambda-readback checks passed.") (newline)
