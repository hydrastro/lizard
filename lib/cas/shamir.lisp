; -*- lisp -*-
; lib/cas/shamir.lisp -- Shamir (t, n) threshold secret sharing over F_p.
;
; A secret s in F_p is hidden as the constant term of a random polynomial
; P(x) = s + a_1 x + ... + a_{t-1} x^{t-1}; the n shares are the evaluations (i, P(i)) for
; i = 1..n.  Any t shares determine P by Lagrange interpolation, so the secret is recovered
; as P(0); any t-1 shares leave P(0) completely undetermined -- for every candidate secret
; there is a polynomial of degree t-1 through those shares hitting it at 0.
;
; Two independent facts certify the scheme: reconstruction from any t of the n shares
; returns the original secret (checked across several different t-subsets), and the
; security property holds -- given only t-1 shares, two distinct secrets each admit a
; consistent interpolating polynomial reproducing exactly those shares, so t-1 shares carry
; no information about the secret.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (full-coeffs secret rest) (cons secret rest))           ; a_0 = secret, then a_1..a_{t-1}
(define (poly-eval c x p) (if (null? c) 0 (imod (+ (car c) (* x (poly-eval (cdr c) x p))) p)))
(define (mk shares-coeffs i n p) (if (> i n) '() (cons (cons i (poly-eval shares-coeffs i p)) (mk shares-coeffs (+ i 1) n p))))
(define (make-shares secret rest n p) (mk (full-coeffs secret rest) 1 n p))

; ---------- Lagrange interpolation of a point set, evaluated at x0 ----------
(define (lbasis-go pi pts x0 p acc)
  (cond ((null? pts) acc)
        ((= (car (car pts)) (car pi)) (lbasis-go pi (cdr pts) x0 p acc))
        (else (lbasis-go pi (cdr pts) x0 p
                (imod (* acc (* (imod (- x0 (car (car pts))) p)
                                (mod-inverse (imod (- (car pi) (car (car pts))) p) p))) p)))))
(define (lagrange-at points x0 p) (la points points x0 p 0))
(define (la pts all x0 p acc)
  (if (null? pts) acc
      (la (cdr pts) all x0 p (imod (+ acc (* (cdr (car pts)) (lbasis-go (car pts) all x0 p 1))) p))))
(define (reconstruct points p) (lagrange-at points 0 p))

; ---------- list helpers ----------
(define (take-n l n) (if (or (= n 0) (null? l)) '() (cons (car l) (take-n (cdr l) (- n 1)))))
(define (drop-n l n) (if (or (= n 0) (null? l)) l (drop-n (cdr l) (- n 1))))

; ---------- certificates ----------
(define (reconstruct-ok? secret rest n p)
  (let ((shares (make-shares secret rest n p)) (t (length (full-coeffs secret rest))))
    (and (= (reconstruct (take-n shares t) p) secret)
         (= (reconstruct (drop-n shares (- n t)) p) secret)
         (= (reconstruct (take-n (drop-n shares 1) t) p) secret))))
(define (reproduces full pts p)
  (cond ((null? pts) #t)
        ((= (lagrange-at full (car (car pts)) p) (cdr (car pts))) (reproduces full (cdr pts) p))
        (else #f)))
(define (consistent partial s p)                       ; (partial + (0,s)) reproduces partial and gives s at 0
  (let ((full (cons (cons 0 s) partial)))
    (and (= (lagrange-at full 0 p) s) (reproduces full partial p))))
(define (security-ok? secret rest n p s1 s2)
  (let ((shares (make-shares secret rest n p)) (t (length (full-coeffs secret rest))))
    (let ((partial (take-n shares (- t 1))))
      (and (not (= s1 s2)) (consistent partial s1 p) (consistent partial s2 p)))))

; ---------- display ----------
(define (share->string sh) (string-append "(" (number->string (car sh)) ", " (number->string (cdr sh)) ")"))
(define (shares->string shs) (if (null? shs) "" (if (null? (cdr shs)) (share->string (car shs)) (string-append (share->string (car shs)) " " (shares->string (cdr shs))))))
