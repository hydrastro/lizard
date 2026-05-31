;; 137-cubical-paths.lisp
;;
;; The cubical layer (Phase 7, TT2 milestone): an interval `I` with endpoints
;; `i0`/`i1`, path types `Path A a b`, path abstraction `(plam i body)`, and
;; path application `(papp p r)`.  Two computation rules: beta
;; `(plam i. t) @ r = t[i:=r]`, and the boundary `p @ i0 = a`, `p @ i1 = b`,
;; which holds even for a NEUTRAL path because the endpoints are recovered from
;; the path's type.  `refl` is just the constant path `plam i. a`, and function
;; application lifts to paths as `ap f p = plam i. f (p @ i)`.
;;
;; (Kan operations / transport / univalence are later milestones and are not
;; part of this fragment, which is sound on its own.)
;;
;; Self-checking via `must`.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'cubical-regression)))

(kernel-assume 'A '(Sort 0))
(kernel-assume 'x 'A)
(kernel-assume 'y 'A)

(display "Path types and refl-as-a-constant-path:") (newline)
(must "plam i. x  :  Path A x x"
  (kernel-check '(plam i x) '(Path A x x)))
(must "an ill-typed path (wrong endpoints) is rejected"
  (not (kernel-check '(plam i x) '(Path A x y))))

(display "The boundary holds even for a neutral path:") (newline)
(kernel-assume 'p '(Path A x y))
(must "p @ i0 = x  (left endpoint)"
  (kernel-check '(refl x) '(Id A (papp p i0) x)))
(must "p @ i1 = y  (right endpoint)"
  (kernel-check '(refl y) '(Id A (papp p i1) y)))

(display "Path application beta-reduces:") (newline)
(must "(plam i. i) @ i1 = i1"
  (kernel-check '(refl i1) '(Id I (papp (plam i i) i1) i1)))

(display "Function application lifts to paths (general ap):") (newline)
(kernel-assume 'B '(Sort 0))
(kernel-assume 'f '(Pi (z A) B))
(must "ap : {A B} (f:A->B) {a b} -> Path A a b -> Path B (f a) (f b)"
  (kernel-define 'ap
    '(lam (A (Sort 0)) (lam (B (Sort 0)) (lam (f (Pi (z A) B))
       (lam (a A) (lam (b A) (lam (p (Path A a b))
          (plam i (app f (papp p i)))))))))
    '(IPi (A (Sort 0)) (IPi (B (Sort 0)) (Pi (f (Pi (z A) B))
       (IPi (a A) (IPi (b A) (Pi (p (Path A a b)) (Path B (app f a) (app f b))))))))))
;; the lifted path computes: its left endpoint is f x
(must "(ap f p) @ i0 = f x  (ap computes through beta + boundary)"
  (kernel-check '(refl (app f x))
    '(Id B (papp (app (app (app (app (app (app ap A) B) f) x) y) p) i0) (app f x))))
;; ap of a constant path is a constant path: ap f (plam i.x) : Path B (f x)(f x)
(must "ap f (refl x) : Path B (f x) (f x)"
  (kernel-check '(app (app (app (app (app (app ap A) B) f) x) x) (plam i x))
    '(Path B (app f x) (app f x))))

(newline)
(display "Interval, Path, path abstraction/application: beta and boundary compute.")
(newline)
