; -*- lisp -*-
; lib/cas/sternbrocot.lisp -- the Stern-Brocot tree of the positive rationals.
;
; Every positive rational appears exactly once in the Stern-Brocot tree and is reached by a
; unique finite path of Left/Right moves from the root 1/1.  Starting from the boundary
; fractions 0/1 and 1/0, each node is their mediant (a+c)/(b+d); comparing the target to
; the mediant says whether to descend left or right, and the search ends when the mediant
; equals the target.  Reversing the moves reconstructs the rational from its path.
;
; Three independent certificates witness the structure:
;   * round trip -- reconstructing the path's rational returns the original;
;   * the Farey-neighbour invariant -- at every node the two bounding fractions a/b and c/d
;     satisfy c*b - a*d = 1, the defining unimodular relation of the tree (so every node is
;     automatically a fraction in lowest terms);
;   * level distinctness -- the 2^k rationals at depth k are pairwise distinct, witnessing
;     that no rational repeats.
; The run-length encoding of a path is also exposed (its link to continued fractions).
; Self-contained over the rationals.

(define (sb-num x) (numerator x))
(define (sb-den x) (denominator x))
(define (mediant a b) (/ (+ (numerator a) (numerator b)) (+ (denominator a) (denominator b))))

; ---------- path to a positive rational ----------
(define (sb-path x) (sbp (numerator x) (denominator x) 0 1 1 0 '()))
(define (sbp pn pd ln ld hn hd acc)
  (let ((mn (+ ln hn)) (md (+ ld hd)))
    (cond ((= (* pn md) (* mn pd)) (reverse acc))
          ((< (* pn md) (* mn pd)) (sbp pn pd ln ld mn md (cons 'L acc)))
          (else (sbp pn pd mn md hn hd (cons 'R acc))))))

; ---------- rational from a path ----------
(define (sb-from-path path) (sbf path 0 1 1 0))
(define (sbf path ln ld hn hd)
  (let ((mn (+ ln hn)) (md (+ ld hd)))
    (cond ((null? path) (/ mn md))
          ((equal? (car path) 'L) (sbf (cdr path) ln ld mn md))
          (else (sbf (cdr path) mn md hn hd)))))

(define (sb-depth x) (length (sb-path x)))

; ---------- run-length encoding of a path (R a0, L a1, R a2, ...) ----------
(define (sb-runs x) (rle (sb-path x)))
(define (rle p) (if (null? p) '() (rle-go (car p) 1 (cdr p))))
(define (rle-go cur n rest) (cond ((null? rest) (list n)) ((equal? (car rest) cur) (rle-go cur (+ n 1) (cdr rest))) (else (cons n (rle-go (car rest) 1 (cdr rest))))))

; ---------- enumeration of a level ----------
(define (all-paths k) (if (= k 0) (list '()) (let ((rest (all-paths (- k 1)))) (append (map (lambda (p) (cons 'L p)) rest) (map (lambda (p) (cons 'R p)) rest)))))
(define (level-rationals k) (map sb-from-path (all-paths k)))

; ---------- certificates ----------
(define (sb-roundtrip-ok? x) (= (sb-from-path (sb-path x)) x))
(define (sb-farey-ok? x) (sf (sb-path x) 0 1 1 0))
(define (sf path ln ld hn hd)
  (if (not (= (- (* hn ld) (* ln hd)) 1)) #f
    (let ((mn (+ ln hn)) (md (+ ld hd)))
      (cond ((null? path) #t)
            ((equal? (car path) 'L) (sf (cdr path) ln ld mn md))
            (else (sf (cdr path) mn md hn hd))))))
(define (member-eq? x l) (cond ((null? l) #f) ((= x (car l)) #t) (else (member-eq? x (cdr l)))))
(define (distinct? l) (cond ((null? l) #t) ((member-eq? (car l) (cdr l)) #f) (else (distinct? (cdr l)))))
(define (reduced? x) (= (gcd (numerator x) (denominator x)) 1))
(define (all-reduced? l) (cond ((null? l) #t) ((reduced? (car l)) (all-reduced? (cdr l))) (else #f)))
(define (level-distinct-ok? k) (let ((l (level-rationals k))) (and (distinct? l) (= (length l) (expt 2 k)) (all-reduced? l))))

; ---------- display ----------
(define (path->string p) (if (null? p) "root" (p->s p)))
(define (p->s p) (cond ((null? p) "") ((equal? (car p) 'L) (string-append "L" (p->s (cdr p)))) (else (string-append "R" (p->s (cdr p))))))
(define (sb->string x) (string-append (number->string (numerator x)) "/" (number->string (denominator x)) " : " (path->string (sb-path x))))
