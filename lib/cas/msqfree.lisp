; -*- lisp -*-
; lib/cas/msqfree.lisp — squarefree factorization of bivariate polynomials over Q,
; by Yun's algorithm built on the bivariate GCD (mgcd.lisp).
;
; For f(x,y) in Q[x,y], Yun's algorithm separates the repeated factors using
; gcd(f, df/dx): it produces a list of pairwise-coprime, squarefree factors
; a_1, a_2, ... with f = prod a_i^i (up to a constant).  Each step is a bivariate gcd
; and an exact bivariate division (both over Q(y)[x], cleared to Q[x,y]).
;
;   c = gcd(f, f');  w = f/c;  y = f'/c;  i = 1
;   repeat:  z = y - w';  g = gcd(w, z)  [the factor of multiplicity i];
;            w = w/g;  y = z/g;  i = i+1   until w is constant.
;
; The factorization is checked by reconstruction (prod a_i^i must equal f up to a
; constant) and by confirming each a_i is squarefree (gcd(a_i, a_i') is constant).
;
; Builds on mgcd.lisp (and thus poly.lisp).  Representation: f is a list of Q[y]
; coefficients in x, low-to-high (the Q[y][x] representation from mgcd.lisp).

(import "cas/mgcd.lisp")

; ---------- d/dx and Q[y][x] helpers ----------
(define (xyd f k) (if (null? f) '() (cons (poly-scale k (car f)) (xyd (cdr f) (+ k 1)))))
(define (xy-deriv-x f) (if (or (null? f) (null? (cdr f))) '() (xyd (cdr f) 1)))
(define (xy-trim f) (reverse (xy-dropz (reverse f))))
(define (xy-dropz f) (cond ((null? f) '()) ((poly-zero? (car f)) (xy-dropz (cdr f))) (else f)))
(define (xy-degx f) (- (length (xy-trim f)) 1))
(define (xy-zero? f) (null? (xy-trim f)))
(define (xy-const? f) (let ((t (xy-trim f))) (cond ((null? t) #t) ((> (length t) 1) #f) (else (<= (poly-deg (car t)) 0)))))
(define (xy-sub f g) (let ((n (max (length f) (length g)))) (xy-sub2 (xy-pad f n) (xy-pad g n))))
(define (xy-sub2 f g) (if (null? f) '() (cons (poly-sub (car f) (car g)) (xy-sub2 (cdr f) (cdr g)))))

; ---------- exact division over Q(y)[x], cleared to Q[x,y] ----------
(define (xq-add f g) (let ((n (max (length f) (length g)))) (xq-add2 (xq-pad f n) (xq-pad g n))))
(define (xq-add2 f g) (if (null? f) '() (cons (qf-add (car f) (car g)) (xq-add2 (cdr f) (cdr g)))))
(define (xq-divmod f g)
  (let ((ft (xq-trim f)))
    (if (or (null? ft) (< (xq-deg ft) (xq-deg g))) (cons '() ft)
        (let ((s (- (xq-deg ft) (xq-deg g))) (lc (qf-div (xq-lead ft) (xq-lead g))))
          (let ((rest (xq-divmod (xq-trim (xq-sub ft (xq-scale-shift g lc s))) g)))
            (cons (xq-add (car rest) (append (zeros-q s) (list lc))) (cdr rest)))))))
(define (xy-quotient f g) (clear-denoms (car (xq-divmod (embed f) (embed g)))))

; ---------- Yun's squarefree factorization -> list of (factor . multiplicity) ----------
(define (sqfree f)
  (let ((fp (xy-deriv-x f)))
    (if (xy-zero? fp) (list (cons (xy-normalize f) 1))
        (let ((c (mgcd f fp)))
          (xy-yun-loop (xy-quotient f c) (xy-quotient fp c) 1 '())))))
(define (xy-yun-loop w y i acc)
  (let ((g (mgcd w (xy-sub y (xy-deriv-x w)))))
    (let ((w2 (xy-quotient w g)) (acc2 (if (xy-const? g) acc (append acc (list (cons (xy-normalize g) i))))))
      (if (xy-const? w2) acc2 (xy-yun-loop w2 (xy-quotient (xy-sub y (xy-deriv-x w)) g) (+ i 1) acc2)))))

; ---------- reconstruction and certificate ----------
(define (xy-pow f k) (if (= k 0) (list (list 1)) (xy-mul f (xy-pow f (- k 1)))))
(define (reconstruct facts) (if (null? facts) (list (list 1)) (xy-mul (xy-pow (car (car facts)) (cdr (car facts))) (reconstruct (cdr facts)))))
(define (sqfree-ok? f facts)
  (and (equal? (xy-normalize (xy-trim (reconstruct facts))) (xy-normalize (xy-trim f)))
       (all-squarefree? facts)))
(define (all-squarefree? facts) (cond ((null? facts) #t) ((xy-const? (mgcd (car (car facts)) (xy-deriv-x (car (car facts))))) (all-squarefree? (cdr facts))) (else #f)))

; ---------- display ----------
(define (sqfree->string facts) (if (null? facts) "1" (sf-go facts "")))
(define (sf-go facts acc)
  (if (null? facts) acc
    (let ((piece (string-append "(" (xy->string (car (car facts))) ")^" (number->string (cdr (car facts))))))
      (sf-go (cdr facts) (if (equal? acc "") piece (string-append acc " * " piece))))))
