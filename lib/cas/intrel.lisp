; -*- lisp -*-
; lib/cas/intrel.lisp -- integer relation detection via LLL.
;
; Given rationals x_1,...,x_n, an integer relation is a nonzero integer vector a with
; a_1 x_1 + ... + a_n x_n = 0.  This is the classic application of lattice reduction: embed the
; problem as the lattice spanned by the rows (e_i | C*x_i), an identity block beside a scaled
; copy of the data.  A lattice vector is (a, C*(a.x)); making it short forces a.x toward zero,
; and for a large enough weight C the shortest reduced vectors have a.x exactly zero, with the
; identity block exposing the relation a.  Because the basis entries can be rational and the LLL
; change of basis is integer-unimodular, the first n coordinates of any reduced vector are
; integers, so the recovered a is automatically an integer vector.  The result is self-checking:
; only an a with a.x = 0 exactly (and a nonzero) is ever returned.  Builds on lll.lisp.

(import "cas/lll.lisp")

; ---------- flat-vector helpers ----------
(define (ir-nth v i) (if (= i 0) (car v) (ir-nth (cdr v) (- i 1))))
(define (ir-len v) (if (null? v) 0 (+ 1 (ir-len (cdr v)))))
(define (ir-e i n) (ir-e-go 0 n i))
(define (ir-e-go j n i) (if (>= j n) '() (cons (if (= j i) 1 0) (ir-e-go (+ j 1) n i))))
(define (ir-take v k) (if (= k 0) '() (cons (car v) (ir-take (cdr v) (- k 1)))))
(define (ir-nonzero? a) (cond ((null? a) #f) ((not (= (car a) 0)) #t) (else (ir-nonzero? (cdr a)))))

; ---------- the LLL embedding ----------
(define (ir-embed x C n) (ir-rows x C n 0))
(define (ir-rows x C n i) (if (>= i n) '() (cons (append (ir-e i n) (list (* C (ir-nth x i)))) (ir-rows x C n (+ i 1)))))

; ---------- search reduced rows for an exact relation ----------
(define (ir-better a best) (cond ((null? best) a) ((< (vnorm2 a) (vnorm2 best)) a) (else best)))
(define (ir-scan rows x n best)
  (if (null? rows) best
      (let ((a (ir-take (car rows) n)))
        (if (and (ir-nonzero? a) (= (vdot a x) 0))
            (ir-scan (cdr rows) x n (ir-better a best))
            (ir-scan (cdr rows) x n best)))))

; try increasing weights C until a genuine relation surfaces (verified exactly)
(define (ir-relation x) (ir-try x (ir-len x) 1000 6))
(define (ir-try x n C tries)
  (if (= tries 0) 'none
      (let ((cand (ir-scan (lll (ir-embed x C n)) x n '())))
        (if (null? cand) (ir-try x n (* C 1000) (- tries 1)) cand))))

; ---------- certificates ----------
(define (ir-dot a x) (vdot a x))
(define (ir-relation-ok? x)
  (let ((a (ir-relation x))) (and (not (equal? a 'none)) (ir-nonzero? a) (= (vdot a x) 0))))
(define (ir-verify a x) (and (ir-nonzero? a) (= (vdot a x) 0)))   ; check a supplied relation
