; -*- lisp -*-
; lib/cas/quadforms.lisp -- binary quadratic forms: reduction and class numbers.
;
; A binary quadratic form a x^2 + b xy + c y^2 is written (a b c); its discriminant is
; D = b^2 - 4ac.  For D < 0 and a > 0 the form is positive definite, and Gauss reduction
; brings it to the unique equivalent REDUCED form with -a < b <= a <= c (and b >= 0 when
; a = c), using two moves: a swap S = (0 -1; 1 0) sending (a,b,c) to (c,-b,a) when a > c,
; and a translation T^k normalising b into (-a, a].  The whole SL2(Z) transformation M is
; accumulated so the reduction carries its own proof.
;
; The class number h(D) is the number of primitive reduced forms of discriminant D,
; obtained by direct enumeration (every reduced form has a <= sqrt(-D/3)).  Reduction is
; certified four independent ways: the discriminant is unchanged, the output satisfies the
; reduced predicate, det(M) = 1, and applying M to the original form actually yields the
; reduced form.  So h(-4) = 1, h(-23) = 3, h(-47) = 5, and the Heegner discriminant
; h(-163) = 1.  Self-contained over the integers.

(define (fa f) (car f))
(define (fb f) (car (cdr f)))
(define (fc f) (car (cdr (cdr f))))
(define (disc f) (- (* (fb f) (fb f)) (* 4 (fa f) (fc f))))

; ---------- apply a matrix M = (p q r s) to a form, x = p x' + q y', y = r x' + s y' ----------
(define (form-apply a b c p q r s)
  (list (+ (* a p p) (* b p r) (* c r r))
        (+ (* 2 a p q) (* b (+ (* p s) (* q r))) (* 2 c r s))
        (+ (* a q q) (* b q s) (* c s s))))

; ---------- normalise b into (-a, a] ----------
(define (normb b a)
  (let ((r (remainder b (* 2 a))))
    (cond ((> r a) (- r (* 2 a))) ((<= r (- 0 a)) (+ r (* 2 a))) (else r))))

; ---------- Gauss reduction, threading the transformation matrix M = (p q r s) ----------
(define (reduce-tracked a b c p q r s)
  (let ((bp (normb b a)))
    (if (not (= bp b))
        (let ((k (quotient (- b bp) (* 2 a))))                 ; b -> b - 2 a k  (translation T^-k)
          (reduce-tracked a bp (+ (* a k k) (- 0 (* b k)) c) p (+ (* (- 0 k) p) q) r (+ (* (- 0 k) r) s)))
        (cond ((> a c) (reduce-tracked c (- 0 b) a q (- 0 p) s (- 0 r)))   ; swap S
              ((and (= a c) (< b 0)) (reduce-tracked a (- 0 b) c q (- 0 p) s (- 0 r)))
              (else (list a b c p q r s))))))
(define (reduce-form f) (let ((g (reduce-tracked (fa f) (fb f) (fc f) 1 0 0 1))) (list (car g) (car (cdr g)) (car (cdr (cdr g))))))
(define (reduce-matrix f) (let ((g (reduce-tracked (fa f) (fb f) (fc f) 1 0 0 1))) (cdr (cdr (cdr g)))))

; ---------- reduced predicate ----------
(define (reduced? f) (let ((a (fa f)) (b (fb f)) (c (fc f)))
  (and (< (- 0 a) b) (<= b a) (<= a c) (if (or (= a c) (= b a)) (>= b 0) #t))))

; ---------- class number: primitive reduced forms of discriminant D < 0 ----------
(define (g3 a b c) (gcd (gcd a b) c))
(define (form-count D a b)
  (if (not (= (remainder (- (* b b) D) (* 4 a)) 0)) 0
    (let ((c (quotient (- (* b b) D) (* 4 a))))
      (if (and (>= c a) (= (g3 a b c) 1) (if (= a c) (>= b 0) #t)) 1 0))))
(define (cn-b D a b) (if (> b a) 0 (+ (form-count D a b) (cn-b D a (+ b 1)))))
(define (cn-a D a) (if (> (* 3 a a) (- 0 D)) 0 (+ (cn-b D a (+ (- 0 a) 1)) (cn-a D (+ a 1)))))
(define (class-number D) (cn-a D 1))

; ---------- certificates ----------
(define (disc-invariant-ok? f) (= (disc (reduce-form f)) (disc f)))
(define (reduce-is-reduced? f) (reduced? (reduce-form f)))
(define (det-ok? f) (let ((m (reduce-matrix f))) (= (- (* (car m) (car (cdr (cdr (cdr m))))) (* (car (cdr m)) (car (cdr (cdr m))))) 1)))
(define (transform-ok? f)
  (let ((m (reduce-matrix f)))
    (equal? (form-apply (fa f) (fb f) (fc f) (car m) (car (cdr m)) (car (cdr (cdr m))) (car (cdr (cdr (cdr m))))) (reduce-form f))))
(define (reduce-ok? f) (and (disc-invariant-ok? f) (reduce-is-reduced? f) (det-ok? f) (transform-ok? f)))

; ---------- display ----------
(define (form->string f) (string-append (number->string (fa f)) " " (number->string (fb f)) " " (number->string (fc f))))
