; -*- lisp -*-
; lib/cas/ec.lisp -- elliptic curves over a prime field F_p.
;
; A curve y^2 = x^3 + a x + b over F_p (with 4a^3 + 27b^2 /= 0, so it is nonsingular)
; carries a group law on its points -- the affine solutions (x . y) together with a point
; at infinity O acting as identity.  For P /= +-Q the sum uses the chord slope
; (y2 - y1)/(x2 - x1); for P = Q the tangent slope (3x1^2 + a)/(2y1); and P + (-P) = O.
; Scalar multiplication is double-and-add.  The number of points is p + 1 + sum over x of
; the Legendre symbol of x^3 + a x + b, and an element's order is the least k with kP = O.
;
; The implementation is held to the defining structure, every clause an independent check:
; the curve is nonsingular; the sum of two points is again on the curve (closure); the law
; is associative ((P+Q)+R = P+(Q+R)); O is an identity and -P an inverse; the count obeys
; the Hasse bound |#E - (p+1)| <= 2 sqrt p; and the order of every point divides #E
; (Lagrange).  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (f-val a b x p) (imod (+ (* x x x) (* a x) b) p))
(define (nonsingular? a b p) (not (= (imod (+ (* 4 a a a) (* 27 b b)) p) 0)))
(define (on-curve? P a b p) (or (equal? P 'O) (= (imod (* (cdr P) (cdr P)) p) (f-val a b (car P) p))))

; ---------- the group law ----------
(define (ec-neg P p) (if (equal? P 'O) 'O (cons (car P) (imod (- 0 (cdr P)) p))))
(define (ec-double P a p)
  (if (= (imod (cdr P) p) 0) 'O
    (let ((lam (imod (* (+ (* 3 (car P) (car P)) a) (mod-inverse (imod (* 2 (cdr P)) p) p)) p)))
      (let ((x3 (imod (- (* lam lam) (* 2 (car P))) p)))
        (cons x3 (imod (- (* lam (- (car P) x3)) (cdr P)) p))))))
(define (ec-add-distinct P Q p)
  (let ((lam (imod (* (- (cdr Q) (cdr P)) (mod-inverse (imod (- (car Q) (car P)) p) p)) p)))
    (let ((x3 (imod (- (- (* lam lam) (car P)) (car Q)) p)))
      (cons x3 (imod (- (* lam (- (car P) x3)) (cdr P)) p)))))
(define (ec-add P Q a p)
  (cond ((equal? P 'O) Q)
        ((equal? Q 'O) P)
        ((= (imod (+ (cdr P) (cdr Q)) p) 0) (if (= (car P) (car Q)) 'O (ec-add-distinct P Q p)))
        ((and (= (car P) (car Q)) (= (cdr P) (cdr Q))) (ec-double P a p))
        (else (ec-add-distinct P Q p))))
(define (ec-mul n P a p)
  (if (= n 0) 'O
    (let ((Q (ec-double-or (ec-mul (quotient n 2) P a p) a p)))
      (if (= (imod n 2) 1) (ec-add Q P a p) Q))))
(define (ec-double-or Q a p) (ec-add Q Q a p))

; ---------- enumeration, counting, order ----------
(define (legendre a p) (let ((r (imod a p))) (cond ((= r 0) 0) ((= (mod-exp r (quotient (- p 1) 2) p) 1) 1) (else -1))))
(define (sum-leg a b p x acc) (if (>= x p) acc (sum-leg a b p (+ x 1) (+ acc (legendre (f-val a b x p) p)))))
(define (ec-count a b p) (+ (+ p 1) (sum-leg a b p 0 0)))
(define (y-list a b p x) (yl a b p x 0 '()))
(define (yl a b p x y acc) (cond ((>= y p) (reverse acc)) ((= (imod (* y y) p) (f-val a b x p)) (yl a b p x (+ y 1) (cons (cons x y) acc))) (else (yl a b p x (+ y 1) acc))))
(define (affine a b p x acc) (if (>= x p) acc (affine a b p (+ x 1) (append acc (y-list a b p x)))))
(define (ec-points a b p) (cons 'O (affine a b p 0 '())))
(define (ec-order P a p) (eco P P a p 1))
(define (eco P cur a p k) (if (equal? cur 'O) k (eco P (ec-add cur P a p) a p (+ k 1))))

; ---------- certificates ----------
(define (hasse-ok? a b p) (let ((t (- (ec-count a b p) (+ p 1)))) (<= (* t t) (* 4 p))))
(define (closure-pair P Q a b p) (on-curve? (ec-add P Q a p) a b p))
(define (closure-row P pts a b p) (cond ((null? pts) #t) ((closure-pair P (car pts) a b p) (closure-row P (cdr pts) a b p)) (else #f)))
(define (closure-all pts all a b p) (cond ((null? pts) #t) ((closure-row (car pts) all a b p) (closure-all (cdr pts) all a b p)) (else #f)))
(define (ec-closure-ok? a b p) (let ((pts (ec-points a b p))) (closure-all pts pts a b p)))
(define (assoc-triple P Q R a p) (equal? (ec-add (ec-add P Q a p) R a p) (ec-add P (ec-add Q R a p) a p)))
(define (assoc-k P Q pts a p) (cond ((null? pts) #t) ((assoc-triple P Q (car pts) a p) (assoc-k P Q (cdr pts) a p)) (else #f)))
(define (assoc-j P pts all a p) (cond ((null? pts) #t) ((assoc-k P (car pts) all a p) (assoc-j P (cdr pts) all a p)) (else #f)))
(define (ec-assoc-ok? a b p) (let ((pts (ec-points a b p))) (assoc-i pts pts a p)))
(define (assoc-i pts all a p) (cond ((null? pts) #t) ((assoc-j (car pts) all all a p) (assoc-i (cdr pts) all a p)) (else #f)))
(define (ec-inverse-ok? a b p) (let ((pts (ec-points a b p))) (inv-all pts a p)))
(define (inv-all pts a p) (cond ((null? pts) #t) ((equal? (ec-add (car pts) (ec-neg (car pts) p) a p) 'O) (inv-all (cdr pts) a p)) (else #f)))
(define (ec-lagrange-ok? a b p) (let ((n (ec-count a b p))) (lagrange-all (ec-points a b p) n a p)))
(define (lagrange-all pts n a p) (cond ((null? pts) #t) ((equal? (car pts) 'O) (lagrange-all (cdr pts) n a p)) ((= (imod n (ec-order (car pts) a p)) 0) (lagrange-all (cdr pts) n a p)) (else #f)))

; ---------- display ----------
(define (pt->string P) (if (equal? P 'O) "O" (string-append "(" (number->string (car P)) ", " (number->string (cdr P)) ")")))
(define (curve->string a b p) (string-append "y^2 = x^3 + " (number->string a) "x + " (number->string b) " over F_" (number->string p)))
