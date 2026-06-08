; -*- lisp -*-
; lib/cas/poly.lisp — univariate polynomial algebra over the rationals.
;
; A polynomial is a DENSE coefficient list, low-to-high: the list (a0 a1 a2 ...)
; denotes a0 + a1 x + a2 x^2 + ....  Coefficients are exact rationals (lizard's
; numbers are exact, with bignum numerators/denominators), so this layer is an
; exact, total CAS substrate.  The zero polynomial is the empty list; every
; result is NORMALISED (no trailing high-degree zeros), which makes structural
; equality on polynomials meaningful.
;
; Everything here is referentially transparent (coefficient lists are immutable
; values), so it composes cleanly and is trivial to certify: an arithmetic
; identity can be checked by evaluating both sides at points, a quotient by
; q*b + r = a, a gcd by divisibility, a square-free split by recombination.
;
; This file deliberately uses only top-level helpers (no internal `define`
; inside `let`, no `let*`) for portability across the interpreter.

; ============================================================
;  small numeric / list helpers the interpreter lacks
; ============================================================
(define (neg x) (- 0 x))
(define (lcm2 a b) (if (or (= a 0) (= b 0)) 0 (quotient (abs (* a b)) (gcd a b))))
(define (nth lst i) (if (= i 0) (car lst) (nth (cdr lst) (- i 1))))
(define (last-elem lst) (if (null? (cdr lst)) (car lst) (last-elem (cdr lst))))
(define (zeros n) (if (= n 0) '() (cons 0 (zeros (- n 1)))))
(define (take-n lst n) (if (= n 0) '() (cons (car lst) (take-n (cdr lst) (- n 1)))))

; ============================================================
;  representation: normalise, degree, leading coeff, accessors
; ============================================================
(define (drop-high-zeros rev)            ; rev is high-to-low; drop leading zeros
  (if (null? rev) '()
    (if (= (car rev) 0) (drop-high-zeros (cdr rev)) rev)))

(define (poly-norm p) (reverse (drop-high-zeros (reverse p))))

(define (poly-zero? p) (null? (poly-norm p)))

(define (poly-deg p) (- (length (poly-norm p)) 1))   ; zero polynomial -> -1

(define (poly-lead p)
  (let ((n (poly-norm p))) (if (null? n) 0 (last-elem n))))

(define (poly-coeff p i)
  (if (or (< i 0) (>= i (length p))) 0 (nth p i)))

(define (const-poly c) (if (= c 0) '() (list c)))

(define (poly-monomial c d) (if (= c 0) '() (append (zeros d) (list c))))

(define (poly-const? p)                  ; degree <= 0
  (let ((n (poly-norm p))) (or (null? n) (null? (cdr n)))))

; ============================================================
;  ring operations
; ============================================================
(define (poly-add-raw p q)
  (cond ((null? p) q)
        ((null? q) p)
        (else (cons (+ (car p) (car q)) (poly-add-raw (cdr p) (cdr q))))))

(define (poly-add p q) (poly-norm (poly-add-raw p q)))
(define (poly-neg p) (map neg p))
(define (poly-sub p q) (poly-norm (poly-add-raw p (poly-neg q))))
(define (poly-scale c p) (if (= c 0) '() (map (lambda (x) (* c x)) p)))

(define (poly-scale-shift a i q)         ; a * x^i * q   (no normalisation)
  (append (zeros i) (map (lambda (c) (* a c)) q)))

(define (poly-mul-acc p q i acc)
  (if (null? p) acc
    (poly-mul-acc (cdr p) q (+ i 1)
                  (poly-add-raw acc (poly-scale-shift (car p) i q)))))

(define (poly-mul p q)
  (if (or (poly-zero? p) (poly-zero? q)) '()
    (poly-norm (poly-mul-acc p q 0 '()))))

(define (poly-pow p n) (if (= n 0) (list 1) (poly-mul p (poly-pow p (- n 1)))))

; Horner evaluation at a rational point
(define (horner hi-lo x acc)
  (if (null? hi-lo) acc (horner (cdr hi-lo) x (+ (* acc x) (car hi-lo)))))
(define (poly-eval p x) (horner (reverse (poly-norm p)) x 0))

; derivative
(define (deriv-terms p i)                ; p starts at the a1 coefficient
  (if (null? p) '() (cons (* i (car p)) (deriv-terms (cdr p) (+ i 1)))))
(define (poly-deriv p)
  (if (null? p) '() (poly-norm (deriv-terms (cdr p) 1))))

(define (poly-monic p)
  (if (poly-zero? p) '() (poly-scale (/ 1 (poly-lead p)) p)))

; ============================================================
;  division with remainder over Q  (a = q*b + r, deg r < deg b)
; ============================================================
(define (poly-divmod-loop r q b lb db)
  (if (< (poly-deg r) db)
      (list (poly-norm q) (poly-norm r))
      (let ((c (/ (poly-lead r) lb)) (d (- (poly-deg r) db)))
        (let ((term (poly-monomial c d)))
          (poly-divmod-loop (poly-sub r (poly-mul term b))
                            (poly-add q term) b lb db)))))

(define (poly-divmod a b)
  (if (poly-zero? b) (raise 'poly-divide-by-zero)
    (poly-divmod-loop (poly-norm a) '() (poly-norm b) (poly-lead b) (poly-deg b))))

(define (poly-div a b) (car (poly-divmod a b)))
(define (poly-rem a b) (car (cdr (poly-divmod a b))))
(define (poly-divides? b a) (poly-zero? (poly-rem a b)))   ; does b divide a?

; ============================================================
;  gcd over Q (Euclid), returned monic
; ============================================================
(define (poly-gcd a b)
  (let ((a (poly-norm a)) (b (poly-norm b)))
    (if (null? b)
        (if (null? a) '() (poly-monic a))
        (poly-gcd b (poly-rem a b)))))

; ============================================================
;  content / primitive part over Z   (clearing denominators)
; ============================================================
(define (fold-lcm lst acc) (if (null? lst) acc (fold-lcm (cdr lst) (lcm2 acc (car lst)))))
(define (fold-gcd lst acc) (if (null? lst) acc (fold-gcd (cdr lst) (gcd acc (car lst)))))

(define (denoms-lcm p) (fold-lcm (map denominator p) 1))
(define (int-content p) (fold-gcd (map (lambda (c) (abs c)) p) 0))

; p = scale * prim, where prim has integer, content-1 coefficients and positive
; leading coefficient.  Returns (list scale prim).
(define (poly-rationalize p)
  (let ((p (poly-norm p)))
    (if (null? p) (list 1 '())
      (let ((cleared (poly-scale (denoms-lcm p) p)))
        (let ((g (int-content cleared)))
          (let ((prim0 (poly-scale (/ 1 g) cleared)))
            (if (negative? (poly-lead prim0))
                (list (/ (neg g) (denoms-lcm p)) (poly-neg prim0))
                (list (/ g (denoms-lcm p)) prim0))))))))

; ============================================================
;  square-free decomposition (Yun's algorithm, char 0)
;  returns list of (list multiplicity monic-factor); p = lead * prod f_i^m_i
; ============================================================
(define (yun-loop b d i acc)
  (if (poly-const? b)
      (reverse acc)
      (let ((ai (poly-gcd b d)))
        (let ((b2 (poly-div b ai)) (c2 (poly-div d ai)))
          (let ((d2 (poly-sub c2 (poly-deriv b2))))
            (yun-loop b2 d2 (+ i 1)
                      (if (poly-const? ai) acc (cons (list i (poly-monic ai)) acc))))))))

(define (square-free p)
  (let ((p (poly-norm p)))
    (if (poly-const? p) '()
      (yun-square-free (poly-monic p)))))

(define (yun-square-free f)              ; f monic, non-constant
  (let ((fp (poly-deriv f)))
    (let ((a0 (poly-gcd f fp)))
      (let ((b1 (poly-div f a0)) (c1 (poly-div fp a0)))
        (yun-loop b1 (poly-sub c1 (poly-deriv b1)) 1 '())))))

; ============================================================
;  s-expression bridge:  (+ a b), (* a b), (^ x n), (- a b), numbers, var
; ============================================================
(define (expr->poly e var)
  (cond ((number? e) (const-poly e))
        ((symbol? e) (if (equal? e var) (list 0 1)
                       (raise (list 'expr->poly-not-univariate e))))
        ((pair? e) (expr-op->poly (car e) (cdr e) var))
        (else (raise (list 'expr->poly-bad e)))))

(define (map-expr->poly args var) (map (lambda (a) (expr->poly a var)) args))

(define (sum-polys ps) (if (null? ps) '() (poly-add (car ps) (sum-polys (cdr ps)))))
(define (prod-polys ps) (if (null? ps) (list 1) (poly-mul (car ps) (prod-polys (cdr ps)))))

(define (expr-op->poly op args var)
  (cond ((equal? op '+) (sum-polys (map-expr->poly args var)))
        ((equal? op '*) (prod-polys (map-expr->poly args var)))
        ((equal? op '-)
         (if (null? (cdr args))
             (poly-neg (expr->poly (car args) var))
             (poly-sub (expr->poly (car args) var)
                       (sum-polys (map-expr->poly (cdr args) var)))))
        ((equal? op '^)
         (poly-pow (expr->poly (car args) var) (car (cdr args))))
        (else (raise (list 'expr->poly-bad-op op)))))

; build a tidy s-expression back from a coefficient list
(define (term->expr c i var)
  (cond ((= i 0) c)
        ((= i 1) (if (= c 1) var (list '* c var)))
        (else (if (= c 1) (list '^ var i) (list '* c (list '^ var i))))))

(define (poly->terms p i var)            ; collect non-zero terms, high to low
  (if (null? p) '()
    (let ((rest (poly->terms (cdr p) (+ i 1) var)))
      (if (= (car p) 0) rest (append rest (list (term->expr (car p) i var)))))))

(define (poly->expr p var)
  (let ((ts (poly->terms (poly-norm p) 0 var)))
    (cond ((null? ts) 0)
          ((null? (cdr ts)) (car ts))
          (else (cons '+ ts)))))

; ============================================================
;  pretty printing:  "x^2 - x - 6",  "1/2*x^3 + ..."
; ============================================================
(define (rat->string c)                  ; number->string is integer-only
  (if (integer? c) (number->string c)
    (string-append (number->string (numerator c)) "/"
                   (number->string (denominator c)))))

(define (mono-abs-string c i var)        ; c is the positive magnitude
  (cond ((= i 0) (rat->string c))
        ((= i 1) (if (= c 1) var (string-append (rat->string c) "*" var)))
        (else (if (= c 1)
                  (string-append var "^" (number->string i))
                  (string-append (rat->string c) "*" var "^" (number->string i))))))

(define (signed-terms p i)               ; nonzero terms as (list coeff degree), low-to-high
  (if (null? p) '()
    (let ((rest (signed-terms (cdr p) (+ i 1))))
      (if (= (car p) 0) rest (cons (list (car p) i) rest)))))

(define (build-poly-str terms first? var)  ; terms high-to-low
  (if (null? terms) (if first? "0" "")
    (let ((c (car (car terms))) (i (car (cdr (car terms)))))
      (let ((body (mono-abs-string (abs c) i var)))
        (string-append
          (cond (first? (if (negative? c) (string-append "-" body) body))
                ((negative? c) (string-append " - " body))
                (else (string-append " + " body)))
          (build-poly-str (cdr terms) #f var))))))

(define (poly->string p var)
  (build-poly-str (reverse (signed-terms (poly-norm p) 0)) #t var))
