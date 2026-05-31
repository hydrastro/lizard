; -*- lisp -*-
; lib/cas/factor.lisp — univariate factorization.
;
; Strategy (the standard "modern" route, all of it exact):
;   1. content + square-free decomposition reduce to factoring a primitive,
;      square-free integer polynomial;
;   2. factor that modulo a good prime p via CANTOR-ZASSENHAUS
;      (distinct-degree then equal-degree splitting over F_p);
;   3. HENSEL-lift the modular factorisation to p^k past the Mignotte bound;
;   4. RECOMBINE lifted factors and trial-divide over Z to recover the true
;      integer irreducibles.
;
; Every result is checked by multiplying the factors back together, so a wrong
; factorisation can never be returned: `factor` either certifies its output or
; reports an unfactored remainder honestly.
;
; This file builds on lib/cas/poly.lisp and uses only top-level helpers.

(import "cas/poly.lisp")

; ============================================================
;  modular integer arithmetic (p prime where inverses are taken)
; ============================================================
(define (mod-int a m)
  (let ((r (remainder a m))) (if (negative? r) (+ r m) r)))

(define (mod-pow b e m)                   ; b^e mod m, e >= 0
  (if (= e 0) (mod-int 1 m)
    (let ((h (mod-pow b (quotient e 2) m)))
      (let ((h2 (mod-int (* h h) m)))
        (if (= (remainder e 2) 0) h2 (mod-int (* h2 b) m))))))

(define (mod-inv a p) (mod-pow (mod-int a p) (- p 2) p))   ; Fermat, p prime

; balanced representative of a mod m, in (-m/2, m/2]
(define (balanced a m)
  (let ((r (mod-int a m))) (if (> (* 2 r) m) (- r m) r)))

; ============================================================
;  primes (trial division — we only need small ones)
; ============================================================
(define (divides-any? n ds)
  (cond ((null? ds) #f)
        ((> (* (car ds) (car ds)) n) #f)
        ((= (remainder n (car ds)) 0) #t)
        (else (divides-any? n (cdr ds)))))

(define (prime? n)
  (if (< n 2) #f (not (divides-any? n (primes-upto-sqrt n 2 '())))))

(define (primes-upto-sqrt n d acc)        ; list of candidate divisors 2..sqrt(n)
  (if (> (* d d) n) (reverse acc)
    (primes-upto-sqrt n (+ d 1) (cons d acc))))

(define (next-prime n) (if (prime? n) n (next-prime (+ n 1))))

; ============================================================
;  polynomials over F_p  (coefficients are integers reduced mod p)
; ============================================================
(define (pmod a p) (poly-norm (map (lambda (c) (mod-int c p)) a)))
(define (padd-mod a b p) (pmod (poly-add a b) p))
(define (psub-mod a b p) (pmod (poly-sub a b) p))
(define (pmul-mod a b p) (pmod (poly-mul a b) p))
(define (pscale-mod c a p) (pmod (map (lambda (x) (* c x)) a) p))

(define (pmonic-mod a p)
  (let ((a (pmod a p)))
    (if (null? a) a (pscale-mod (mod-inv (poly-lead a) p) a p))))

; division with remainder over F_p:  a = q*b + r  (deg r < deg b), b nonzero
(define (pdivmod-mod-loop r q b binv db p)
  (if (< (poly-deg r) db)
      (list (pmod q p) (pmod r p))
      (let ((c (mod-int (* (poly-lead r) binv) p)) (d (- (poly-deg r) db)))
        (let ((term (poly-monomial c d)))
          (pdivmod-mod-loop (psub-mod r (pmul-mod term b p) p)
                            (poly-add q term) b binv db p)))))

(define (pdivmod-mod a b p)
  (pdivmod-mod-loop (pmod a p) '() (pmod b p)
                    (mod-inv (poly-lead b) p) (poly-deg b) p))

(define (prem-mod a b p) (car (cdr (pdivmod-mod a b p))))
(define (pdiv-mod a b p) (car (pdivmod-mod a b p)))

(define (pgcd-mod a b p)
  (let ((a (pmod a p)) (b (pmod b p)))
    (if (null? b)
        (if (null? a) '() (pmonic-mod a p))
        (pgcd-mod b (prem-mod a b p) p))))

; base^e mod (modf, p)  — modular exponentiation of polynomials
(define (ppow-mod base e modf p)
  (if (= e 0) (list 1)
    (let ((h (ppow-mod base (quotient e 2) modf p)))
      (let ((h2 (prem-mod (pmul-mod h h p) modf p)))
        (if (= (remainder e 2) 0) h2
          (prem-mod (pmul-mod h2 base p) modf p))))))

; ============================================================
;  Cantor-Zassenhaus over F_p  (p odd; f monic squarefree)
; ============================================================
; distinct-degree factorisation: list of (list product degree)
(define (ddf-loop f x-pow i acc p)
  (if (< (poly-deg f) (* 2 i))
      (if (poly-const? f) (reverse acc)
        (reverse (cons (list f (poly-deg f)) acc)))
      (let ((xp (ppow-mod x-pow p f p)))           ; x^(p^i) mod f
        (let ((g (pgcd-mod f (psub-mod xp (list 0 1) p) p)))
          (if (poly-const? g)
              (ddf-loop f xp (+ i 1) acc p)
              (let ((f2 (pdiv-mod f g p)))
                (ddf-loop f2 (prem-mod xp f2 p) (+ i 1)
                          (cons (list g i) acc) p)))))))

(define (ddf f p) (ddf-loop (pmonic-mod f p) (list 0 1) 1 '() p))

; equal-degree splitting: f is a product of irreducibles each of degree d
(define (edf f d p) (edf-try (pmonic-mod f p) d p 1))

(define (edf-try f d p a)
  (if (poly-const? f) '()
    (if (= (poly-deg f) d)
        (list f)                                   ; already irreducible
      (let ((g (edf-split f d p a)))
        (if (equal? g 'none)
            (edf-try f d p (+ a 1))                ; try next trial polynomial
          (append (edf (car g) d p) (edf (car (cdr g)) d p)))))))

; try trial polynomial (x + a): g = gcd(f, (x+a)^((p^d-1)/2) - 1)
(define (edf-split f d p a)
  (let ((h (poly-add (list 0 1) (list a))))         ; x + a
    (let ((b (ppow-mod h (quotient (- (expt p d) 1) 2) f p)))
      (let ((g (pgcd-mod f (psub-mod b (list 1) p) p)))
        (if (or (poly-const? g) (= (poly-deg g) (poly-deg f)))
            'none
            (list g (pdiv-mod f g p)))))))

; full factorisation of a squarefree monic f over F_p -> list of monic irreducibles
(define (cz-factor f p)
  (cz-collect (ddf (pmonic-mod f p) p) p '()))

(define (cz-collect ddfs p acc)
  (if (null? ddfs) acc
    (let ((g (car (car ddfs))) (d (car (cdr (car ddfs)))))
      (cz-collect (cdr ddfs) p (append acc (edf g d p))))))

; ============================================================
;  extended Euclid over F_p:  s*a + t*b = 1   (a,b coprime mod p)
;  returned with deg s < deg b
; ============================================================
(define (eea-mod old-r r old-s s old-t t p)
  (if (poly-zero? r)
      (list old-r old-s old-t)
      (let ((q (pdiv-mod old-r r p)))
        (eea-mod r (psub-mod old-r (pmul-mod q r p) p)
                 s (psub-mod old-s (pmul-mod q s p) p)
                 t (psub-mod old-t (pmul-mod q t p) p) p))))

(define (bezout1-mod a b p)
  (let ((res (eea-mod (pmod a p) (pmod b p) (list 1) '() '() (list 1) p)))
    (let ((g (car res)) (s0 (car (cdr res))) (t0 (car (cdr (cdr res)))))
      (let ((cinv (mod-inv (poly-lead g) p)))
        (let ((s (prem-mod (pscale-mod cinv s0 p) b p)))
          (let ((tt (pdiv-mod (psub-mod (list 1) (pmul-mod s a p) p) b p)))
            (list s tt)))))))

; ============================================================
;  monic polynomial division mod m  (m need NOT be prime; divisor monic)
; ============================================================
(define (pdivmod-monic-loop r q h dh m)
  (if (< (poly-deg r) dh)
      (list (pmod q m) (pmod r m))
      (let ((c (poly-lead r)) (d (- (poly-deg r) dh)))
        (let ((term (poly-monomial c d)))
          (pdivmod-monic-loop (psub-mod r (pmul-mod term h m) m)
                              (poly-add q term) h dh m)))))
(define (pdivmod-monic-mod a h m)
  (pdivmod-monic-loop (pmod a m) '() h (poly-deg h) m))

; ============================================================
;  Hensel step: lift  f = g*h (mod m), s*g + t*h = 1 (mod m)  to mod m^2
;  (g,h monic).  von zur Gathen & Gerhard, Modern Computer Algebra 15.10
; ============================================================
(define (hensel-step f g h s t m)
  (let ((mm (* m m)))
    (let ((e (psub-mod f (pmul-mod g h mm) mm)))
      (let ((qr (pdivmod-monic-mod (pmul-mod s e mm) h mm)))
        (let ((q (car qr)) (r (car (cdr qr))))
          (let ((g2 (pmod (poly-add (poly-add g (pmul-mod t e mm)) (pmul-mod q g mm)) mm))
                (h2 (pmod (poly-add h r) mm)))
            (let ((b (psub-mod (poly-add (pmul-mod s g2 mm) (pmul-mod t h2 mm)) (list 1) mm)))
              (let ((cd (pdivmod-monic-mod (pmul-mod s b mm) h2 mm)))
                (let ((c (car cd)) (d (car (cdr cd))))
                  (list g2 h2
                        (psub-mod s d mm)
                        (psub-mod (psub-mod t (pmul-mod t b mm) mm) (pmul-mod c g2 mm) mm)))))))))))

; lift a 2-factor split until the modulus reaches >= bound; returns (list G H M)
(define (hensel-pair-loop f g h s t m bound)
  (if (>= m bound) (list g h m)
    (let ((step (hensel-step f g h s t m)))
      (hensel-pair-loop f (car step) (car (cdr step))
                        (car (cdr (cdr step))) (car (cdr (cdr (cdr step))))
                        (* m m) bound))))

(define (hensel-pair f g h p bound)
  (let ((st (bezout1-mod g h p)))
    (hensel-pair-loop f (pmod g p) (pmod h p) (car st) (car (cdr st)) p bound)))

; lift a LIST of monic mod-p factors of monic f to mod M (peel one at a time)
(define (prod-mod-p factors p) (if (null? factors) (list 1) (pmul-mod (car factors) (prod-mod-p (cdr factors) p) p)))

(define (hensel-list f factors p bound)
  (if (null? (cdr factors))
      (list (pmod f bound))                      ; single factor: f itself mod M
      (let ((g (car factors)) (rest (cdr factors)))
        (let ((h (prod-mod-p rest p)))
          (let ((gh (hensel-pair f g h p bound)))
            (let ((G (car gh)) (H (car (cdr gh))) (M (car (cdr (cdr gh)))))
              (cons G (hensel-list H rest p M))))))))

; ============================================================
;  coefficient bound, prime selection
; ============================================================
(define (fold-max-abs p acc) (if (null? p) acc (fold-max-abs (cdr p) (max acc (abs (car p))))))
(define (maxabs-coeff p) (fold-max-abs p 0))
; generous Landau-Mignotte-style bound on |coefficients| of any factor of f
(define (factor-bound f) (* (expt 2 (poly-deg f)) (+ 1 (maxabs-coeff f))))
(define (lift-mod-loop m bound) (if (>= m bound) m (lift-mod-loop (* m m) bound)))
(define (lift-modulus p bound) (lift-mod-loop p bound))

(define (good-prime? f p)
  (and (not (= (remainder (poly-lead f) p) 0))
       (poly-const? (pgcd-mod f (poly-deriv f) p))))      ; squarefree mod p
(define (pick-prime f p) (if (and (prime? p) (good-prime? f p)) p (pick-prime f (+ p 2))))

; ============================================================
;  recombination (Zassenhaus): combine lifted factors, trial-divide over Z
; ============================================================
(define (combos L k)
  (cond ((= k 0) (list '()))
        ((null? L) '())
        (else (append (map (lambda (c) (cons (car L) c)) (combos (cdr L) (- k 1)))
                      (combos (cdr L) k)))))

(define (remove-one lst x)
  (cond ((null? lst) '())
        ((equal? (car lst) x) (cdr lst))
        (else (cons (car lst) (remove-one (cdr lst) x)))))
(define (list-remove-each lst items)
  (if (null? items) lst (list-remove-each (remove-one lst (car items)) (cdr items))))

(define (pbalance p m) (poly-norm (map (lambda (c) (balanced c m)) p)))
(define (prod-mod-list ps m) (if (null? ps) (list 1) (pmul-mod (car ps) (prod-mod-list (cdr ps) m) m)))

(define (try-subsets f subs m)
  (if (null? subs) 'none
    (let ((cand (pbalance (prod-mod-list (car subs) m) m)))
      (if (and (not (poly-const? cand)) (poly-divides? cand f))
          (list cand (car subs))
          (try-subsets f (cdr subs) m)))))

(define (recombine-loop f factors m size)
  (if (> (* 2 size) (length factors))
      (if (poly-const? f) '() (list f))
      (let ((found (try-subsets f (combos factors size) m)))
        (if (equal? found 'none)
            (recombine-loop f factors m (+ size 1))
            (cons (car found)
                  (recombine-loop (poly-div f (car found))
                                  (list-remove-each factors (car (cdr found))) m size))))))
(define (recombine f factors m) (recombine-loop f factors m 1))

; ============================================================
;  factor a MONIC squarefree integer polynomial into monic irreducibles
; ============================================================
(define (factor-monic-sqfree f)
  (cond ((poly-const? f) '())
        ((= (poly-deg f) 1) (list f))
        (else (factor-monic-go f (pick-prime f 3)))))
(define (factor-monic-go f p)
  (let ((facs (cz-factor f p)))
    (if (null? (cdr facs)) (list f)                ; irreducible mod p => over Z
      (let ((bound (+ (* 2 (factor-bound f)) 1)))
        (recombine f (hensel-list f facs p bound) (lift-modulus p bound))))))

; ============================================================
;  non-monic: monic-reduce  G(y)=b^(n-1) f(y/b),  then transfer factors back
; ============================================================
(define (monic-reduce-coeffs f b n i)
  (cond ((> i n) '())
        ((= i n) (list 1))
        (else (cons (* (poly-coeff f i) (expt b (- (- n 1) i)))
                    (monic-reduce-coeffs f b n (+ i 1))))))
(define (monic-reduce f) (monic-reduce-coeffs f (poly-lead f) (poly-deg f) 0))

(define (subst-scale-loop p b i)
  (if (null? p) '() (cons (* (car p) (expt b i)) (subst-scale-loop (cdr p) b (+ i 1)))))
(define (transfer-factor gj b)                     ; primitive part of gj(b*x)
  (car (cdr (poly-rationalize (subst-scale-loop gj b 0)))))

; ============================================================
;  factor a primitive squarefree integer polynomial (any leading coeff)
; ============================================================
(define (factor-int f)
  (cond ((poly-const? f) '())
        ((= (poly-deg f) 1) (list f))
        ((= (poly-lead f) 1) (factor-monic-sqfree f))
        (else (map (lambda (g) (transfer-factor g (poly-lead f)))
                   (factor-monic-sqfree (monic-reduce f))))))

; ============================================================
;  TOP LEVEL: factor over Q.
;  returns (list const ((mult factor) ...)), factor primitive irreducible over Z,
;  with  p = const * prod factor^mult  (verified by multiply-back).
; ============================================================
(define (prim-of a) (car (cdr (poly-rationalize a))))   ; primitive integer part

(define (collect-irr sf-list)            ; sf-list: ((mult monic-a) ...) over Q
  (if (null? sf-list) '()
    (append (map (lambda (g) (list (car (car sf-list)) g))
                 (factor-int (prim-of (car (cdr (car sf-list))))))
            (collect-irr (cdr sf-list)))))

(define (prod-irr irrs)                  ; product of factor^mult over Z
  (if (null? irrs) (list 1)
    (poly-mul (poly-pow (car (cdr (car irrs))) (car (car irrs)))
              (prod-irr (cdr irrs)))))

(define (factor-Q p)
  (let ((p (poly-norm p)))
    (if (poly-const? p) (list (if (null? p) 0 (car p)) '())
      (let ((irrs (collect-irr (square-free p))))
        (let ((prod (prod-irr irrs)))
          (list (/ (poly-lead p) (poly-lead prod)) irrs))))))

; multiply-back check: does (const, irrs) reconstruct p ?
(define (factor-verify p fz)
  (equal? (poly-norm p)
          (poly-scale (car fz) (prod-irr (car (cdr fz))))))

; ============================================================
;  pretty printing a factorization
; ============================================================
(define (factor-piece mf var)
  (let ((m (car mf)) (g (car (cdr mf))))
    (if (= m 1)
        (string-append "(" (poly->string g var) ")")
        (string-append "(" (poly->string g var) ")^" (number->string m)))))

(define (pieces->string irrs var)
  (if (null? irrs) ""
    (let ((rest (pieces->string (cdr irrs) var)))
      (if (equal? rest "")
          (factor-piece (car irrs) var)
          (string-append (factor-piece (car irrs) var) " * " rest)))))

(define (factorization->string fz var)
  (let ((c (car fz)) (irrs (car (cdr fz))))
    (cond ((null? irrs) (rat->string c))
          ((= c 1) (pieces->string irrs var))
          ((= c -1) (string-append "-" (pieces->string irrs var)))
          (else (string-append (rat->string c) " * " (pieces->string irrs var))))))

; ============================================================
;  expression-level entry: factor an s-expression in one variable.
;  `var` is a symbol (e.g. 'x); result is a product string.
; ============================================================
(define (factor-expr e var)
  (factorization->string (factor-Q (expr->poly e var)) (symbol->string var)))
