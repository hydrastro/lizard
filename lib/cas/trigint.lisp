; -*- lisp -*-
; lib/cas/trigint.lisp -- TRIGONOMETRIC INTEGRATION: closed-form antiderivatives of sin^m(x) cos^n(x) and
; finite sums of such monomials, every result CERTIFIED by differentiating it back to the integrand.
;
; This is a genuinely new integration capability (the rational and algebraic integrators handle R(x) and
; R(x,y) with y algebraic; this handles trigonometric integrands).  The method is the classical reduction:
;   INT sin^m cos^n dx = sin^{m+1} cos^{n-1} / (m+n)  +  (n-1)/(m+n) INT sin^m cos^{n-2} dx     (reduce n), or
;   INT sin^m cos^n dx = -sin^{m-1} cos^{n+1} / (m+n) +  (m-1)/(m+n) INT sin^{m-2} cos^n dx     (reduce m),
; applied until a base case INT 1 dx = x, INT cos dx = sin, or INT sin dx = -cos is reached.  The antiderivative
; is therefore A(s,c) + B*x with A a polynomial in s = sin x, c = cos x and B a rational constant; this shape is
; closed under d/dx (d s = c, d c = -s), so the differentiate-back check is exact.
;
; Representations:
;   trig polynomial = list of terms, each (coeff s-power c-power); reduced to a canonical form with the relation
;     s^2 = 1 - c^2 (so every term has s-power 0 or 1).  trig integral = (cons trigpoly B), meaning A(s,c) + B*x.
;
; Public: ti-integrate m n -> (cons trigpoly B) | trig-integral; ti-deriv (of a trig integral) -> trigpoly;
; ti-canon (canonicalize a trigpoly via s^2=1-c^2); ti-certify m n -> #t iff d/dx(ti-integrate m n) equals the
; integrand sin^m cos^n.  Verified: INT sin^3 = -cos + cos^3/3; INT sin^2 cos = sin^3/3; INT sin^2 cos^2 =
; x/8 + (s^3 c - s c^3)/8; INT cos^2 = x/2 + s c /2; all differentiate back to their integrands.
;
; Self-contained (exact rational arithmetic only).

; ----- trig-polynomial arithmetic: terms (coeff s-pow c-pow) -----
(define (tp-term co sp cp) (list co sp cp))
(define (tp-co t) (car t))
(define (tp-sp t) (car (cdr t)))
(define (tp-cp t) (car (cdr (cdr t))))

(define (tp-add P Q) (tp-collect (append P Q)))
(define (tp-scale a P) (if (null? P) (quote ()) (cons (tp-term (* a (tp-co (car P))) (tp-sp (car P)) (tp-cp (car P))) (tp-scale a (cdr P)))))
(define (tp-mul1 t P) (if (null? P) (quote ()) (cons (tp-term (* (tp-co t) (tp-co (car P))) (+ (tp-sp t) (tp-sp (car P))) (+ (tp-cp t) (tp-cp (car P)))) (tp-mul1 t (cdr P)))))
(define (tp-mul P Q) (if (null? P) (quote ()) (tp-add (tp-mul1 (car P) Q) (tp-mul (cdr P) Q))))

; collect like terms (same s-pow and c-pow), dropping zeros
(define (tp-collect P) (tp-collect-go P (quote ())))
(define (tp-collect-go P acc)
  (if (null? P) (tp-drop0 acc)
      (tp-collect-go (cdr P) (tp-merge (car P) acc))))
(define (tp-merge t acc)
  (if (null? acc) (list t)
      (if (if (= (tp-sp t) (tp-sp (car acc))) (= (tp-cp t) (tp-cp (car acc))) #f)
          (cons (tp-term (+ (tp-co t) (tp-co (car acc))) (tp-sp t) (tp-cp t)) (cdr acc))
          (cons (car acc) (tp-merge t (cdr acc))))))
(define (tp-drop0 P) (if (null? P) (quote ()) (if (= (tp-co (car P)) 0) (tp-drop0 (cdr P)) (cons (car P) (tp-drop0 (cdr P))))))

; canonicalize: rewrite every s^2 -> (1 - c^2) so each term has s-power 0 or 1
(define (ti-canon P) (tp-collect (ti-canon-go P)))
(define (ti-canon-go P)
  (if (null? P) (quote ())
      (append (ti-reduce-term (car P)) (ti-canon-go (cdr P)))))
; reduce one term c0 s^sp c^cp: replace s^(2q+r) by (1-c^2)^q s^r
(define (ti-reduce-term t)
  (let ((co (tp-co t)) (sp (tp-sp t)) (cp (tp-cp t)))
    (let ((q (quotient sp 2)) (r (remainder sp 2)))
      (tp-mul (list (tp-term co r cp)) (ti-pow-1mc2 q)))))
; (1 - c^2)^q as a trig polynomial in c
(define (ti-pow-1mc2 q) (if (= q 0) (list (tp-term 1 0 0)) (tp-mul (list (tp-term 1 0 0) (tp-term -1 0 2)) (ti-pow-1mc2 (- q 1)))))

; derivative of a trig polynomial: d/dx[c0 s^sp c^cp] = c0(sp s^{sp-1} c^{cp+1} - cp s^{sp+1} c^{cp-1})
(define (tp-deriv P) (tp-collect (tp-deriv-go P)))
(define (tp-deriv-go P)
  (if (null? P) (quote ())
      (let ((t (car P)))
        (let ((co (tp-co t)) (sp (tp-sp t)) (cp (tp-cp t)))
          (append
            (append
              (if (= sp 0) (quote ()) (list (tp-term (* co sp) (- sp 1) (+ cp 1))))
              (if (= cp 0) (quote ()) (list (tp-term (* (- 0 co) cp) (+ sp 1) (- cp 1)))))
            (tp-deriv-go (cdr P)))))))

; ----- the reduction-formula integrator for INT sin^m cos^n dx -----
; returns (cons trigpoly B): the antiderivative A(s,c) + B*x.
(define (ti-integrate m n)
  (cond ((if (= m 0) (= n 0) #f) (cons (quote ()) 1))                         ; INT 1 dx = x
        ((if (= m 1) (= n 0) #f) (cons (list (tp-term -1 0 1)) 0))            ; INT sin = -cos
        ((if (= m 0) (= n 1) #f) (cons (list (tp-term 1 1 0)) 0))            ; INT cos = sin
        ((>= n 2)                                                            ; reduce the cos power
         (let ((rest (ti-integrate m (- n 2))))
           (cons (tp-add (list (tp-term (/ 1 (+ m n)) (+ m 1) (- n 1))) (tp-scale (/ (- n 1) (+ m n)) (car rest)))
                 (* (/ (- n 1) (+ m n)) (cdr rest)))))
        ((>= m 2)                                                            ; reduce the sin power
         (let ((rest (ti-integrate (- m 2) n)))
           (cons (tp-add (list (tp-term (/ -1 (+ m n)) (- m 1) (+ n 1))) (tp-scale (/ (- m 1) (+ m n)) (car rest)))
                 (* (/ (- m 1) (+ m n)) (cdr rest)))))
        ((if (= m 1) (= n 1) #f) (cons (list (tp-term (/ 1 2) 2 0)) 0))      ; INT sin cos = sin^2/2
        (else (cons (quote ()) 0))))                                         ; shouldn't reach

; derivative of a trig integral (cons trigpoly B): d/dx[A + Bx] = A' + B
(define (ti-deriv I) (tp-add (tp-deriv (car I)) (if (= (cdr I) 0) (quote ()) (list (tp-term (cdr I) 0 0)))))

; the integrand sin^m cos^n as a (canonicalized) trig polynomial
(define (ti-monomial m n) (ti-canon (list (tp-term 1 m n))))

; CERTIFY: d/dx(ti-integrate m n) equals sin^m cos^n, both canonicalized
(define (ti-certify m n)
  (tp-equal? (ti-canon (ti-deriv (ti-integrate m n))) (ti-monomial m n)))
(define (tp-equal? P Q) (null? (tp-collect (tp-add P (tp-scale -1 Q)))))

; pretty terms (coeff s-pow c-pow) for display, canonicalized
(define (ti-show I) (cons (ti-canon (car I)) (cdr I)))
