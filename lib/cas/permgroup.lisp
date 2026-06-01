; -*- lisp -*-
; lib/cas/permgroup.lisp -- finite permutation groups on {0,1,...,n-1}.
;
; A permutation is the list of images (p k = the element at index k), so the identity on n
; points is (0 1 ... n-1).  Generators generate a subgroup of the symmetric group S_n; the
; whole group is enumerated by a breadth-first search over the Cayley graph (from the identity,
; repeatedly left-multiply by each generator), which visits every element exactly once and so
; gives the group order as a count.  This is then cross-checked against the orbit-stabilizer
; theorem |G| = |orbit(x)| * |stabilizer(x)|, computed independently, and Lagrange's theorem is
; verified for point stabilizers.  Self-contained; no imports.

; ---------- small list helpers ----------
(define (pg-nth l i) (if (= i 0) (car l) (pg-nth (cdr l) (- i 1))))
(define (pg-range a n) (if (>= a n) '() (cons a (pg-range (+ a 1) n))))
(define (pg-member? x l) (cond ((null? l) #f) ((equal? x (car l)) #t) (else (pg-member? x (cdr l)))))
(define (pg-len l) (if (null? l) 0 (+ 1 (pg-len (cdr l)))))
(define (pg-set-at l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (pg-set-at (cdr l) (- i 1) v))))
(define (pg-filter f l) (cond ((null? l) '()) ((f (car l)) (cons (car l) (pg-filter f (cdr l)))) (else (pg-filter f (cdr l)))))
(define (pg-all? f l) (cond ((null? l) #t) ((f (car l)) (pg-all? f (cdr l))) (else #f)))

; ---------- permutation arithmetic ----------
(define (perm-id n) (pg-range 0 n))
(define (perm-apply p i) (pg-nth p i))            ; image of point i
(define (perm-compose p q)                        ; (p after q): result k = p (q k)
  (map (lambda (qk) (pg-nth p qk)) q))
(define (perm-inverse p) (pi-build p 0 (perm-id (pg-len p))))
(define (pi-build p i acc) (if (>= i (pg-len p)) acc (pi-build p (+ i 1) (pg-set-at acc (pg-nth p i) i))))
(define (perm-eq? p q) (equal? p q))
(define (perm-is-id? p) (equal? p (perm-id (pg-len p))))
(define (perm-element-order p) (peo p p 1))       ; least k>0 with p^k = id
(define (peo base cur k) (if (perm-is-id? cur) k (peo base (perm-compose base cur) (+ k 1))))

; ---------- cycle decomposition (for display) ----------
(define (perm-cycles p) (pc p (pg-len p) 0 '() '()))
(define (pc p n i seen acc)
  (cond ((>= i n) (reverse acc))
        ((pg-member? i seen) (pc p n (+ i 1) seen acc))
        (else (let ((cyc (follow p i i (list i))))
                (pc p n (+ i 1) (append seen cyc) (if (null? (cdr cyc)) acc (cons (reverse cyc) acc)))))))
(define (follow p start cur acc) (let ((nx (pg-nth p cur))) (if (= nx start) acc (follow p start nx (cons nx acc)))))

; ---------- group closure: BFS over the Cayley graph ----------
(define (group-closure gens n) (gc-bfs gens n (list (perm-id n)) (list (perm-id n))))
(define (gc-bfs gens n known queue)
  (if (null? queue) known
      (let ((e (car queue)))
        (let ((fresh (gc-new gens e known)))
          (gc-bfs gens n (append known fresh) (append (cdr queue) fresh))))))
(define (gc-new gens e known) (gc-acc gens e known '()))
(define (gc-acc gens e known acc)
  (if (null? gens) (reverse acc)
      (let ((h (perm-compose (car gens) e)))
        (if (or (pg-member? h known) (pg-member? h acc))
            (gc-acc (cdr gens) e known acc)
            (gc-acc (cdr gens) e known (cons h acc))))))
(define (group-order gens n) (pg-len (group-closure gens n)))
(define (group-member? p gens n) (pg-member? p (group-closure gens n)))

; ---------- orbits and stabilizers ----------
(define (orbit x gens n) (orb-bfs gens (list x) (list x)))
(define (orb-bfs gens known queue)
  (if (null? queue) known
      (let ((pt (car queue)))
        (let ((fresh (orb-new gens pt known)))
          (orb-bfs gens (append known fresh) (append (cdr queue) fresh))))))
(define (orb-new gens pt known) (orb-acc gens pt known '()))
(define (orb-acc gens pt known acc)
  (if (null? gens) (reverse acc)
      (let ((y (perm-apply (car gens) pt)))
        (if (or (pg-member? y known) (pg-member? y acc))
            (orb-acc (cdr gens) pt known acc)
            (orb-acc (cdr gens) pt known (cons y acc))))))
(define (stabilizer x gens n) (pg-filter (lambda (p) (= (perm-apply p x) x)) (group-closure gens n)))

; ---------- certificates ----------
(define (divides? a b) (= (imod-pg b a) 0))
(define (imod-pg a b) (- a (* b (quotient a b))))
; closed under all generators (vacuously true if BFS finished, but checked structurally)
(define (closure-closed? gens n)
  (let ((G (group-closure gens n)))
    (pg-all? (lambda (e) (pg-all? (lambda (g) (pg-member? (perm-compose g e) G)) gens)) G)))
(define (closure-has-id? gens n) (pg-member? (perm-id n) (group-closure gens n)))
(define (closure-inverse-closed? gens n)
  (let ((G (group-closure gens n))) (pg-all? (lambda (e) (pg-member? (perm-inverse e) G)) G)))
; orbit-stabilizer: |G| = |orbit(x)| * |stab(x)|
(define (orbit-stabilizer-ok? x gens n)
  (= (group-order gens n) (* (pg-len (orbit x gens n)) (pg-len (stabilizer x gens n)))))
; Lagrange: |stab(x)| divides |G|
(define (lagrange-ok? x gens n) (divides? (pg-len (stabilizer x gens n)) (group-order gens n)))

; ---------- display ----------
(define (perm-info gens n) (string-append "group on " (number->string n) " points, order " (number->string (group-order gens n))))
