; -*- lisp -*-
; lib/cas/ffactor.lisp -- polynomial factorization over a prime field F_p.
;
; Polynomials over F_p are coefficient lists low-to-high, each entry reduced mod p and
; trailing zeros trimmed (so '() is the zero polynomial).  On top of the field arithmetic
; (add, mul, division with remainder, monic gcd, derivative) this implements the full
; Cantor-Zassenhaus factorisation:
;
;   * squarefree decomposition -- f / gcd(f, f') strips repeated factors; when f' = 0 the
;     polynomial is a p-th power h(x)^p and a p-th root is taken (in F_p every coefficient
;     is its own p-th root, so the root just gathers the coefficients at multiples of p);
;   * distinct-degree factorisation -- repeatedly gcd with x^(p^i) - x using the Frobenius
;     map peels off the product of degree-i irreducibles;
;   * equal-degree factorisation -- a block of equal-degree irreducibles is split with the
;     trace map a + a^2 + ... + a^(2^(d-1)) when p = 2, and with a^((p^d - 1)/2) - 1 for
;     odd p, trying a deterministic stream of polynomials so the whole routine is
;     reproducible.
;
; Multiplicities are recovered by dividing the original polynomial; the final factorisation
; is gated two independent ways -- the product of the prime powers must reconstruct the
; monic input mod p, and every returned factor must pass a standalone irreducibility test
; (x^(p^n) = x mod f, and coprimality with x^(p^(n/r)) - x for each prime r | n).  Builds on
; numbertheory.lisp.

(import "cas/numbertheory.lisp")

; ---------- representation helpers ----------
(define (dropz a) (cond ((null? a) '()) ((= (car a) 0) (dropz (cdr a))) (else a)))
(define (trim a) (reverse (dropz (reverse a))))
(define (pnorm a p) (map (lambda (x) (imod x p)) a))
(define (pclean a p) (trim (pnorm a p)))
(define (pdeg a) (- (length (trim a)) 1))                 ; '() -> -1
(define (pzero? a) (null? (trim a)))
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (monomial c s) (if (= c 0) '() (append (zeros s) (list c))))
(define (lead-coef a p) (car (reverse (pclean a p))))
(define (nthc f i) (if (= i 0) (car f) (nthc (cdr f) (- i 1))))

; ---------- field arithmetic on polynomials ----------
(define (addlists a b) (cond ((null? a) b) ((null? b) a) (else (cons (+ (car a) (car b)) (addlists (cdr a) (cdr b))))))
(define (padd a b p) (pclean (addlists a b) p))
(define (pscale a c p) (pclean (map (lambda (x) (* x c)) a) p))
(define (psub a b p) (padd a (pscale b (- p 1) p) p))
(define (pmul a b p) (cond ((null? (trim a)) '()) ((null? (trim b)) '()) (else (pmul-go (trim a) (trim b) p))))
(define (pmul-go a b p) (if (null? a) '() (padd (pscale b (car a) p) (cons 0 (pmul-go (cdr a) b p)) p)))
(define (pmonic a p) (pscale a (mod-inverse (lead-coef a p) p) p))

; ---------- division with remainder ----------
(define (pdivmod f g p) (pdm (pclean f p) (pclean g p) p (mod-inverse (lead-coef g p) p) '()))
(define (pdm f g p ginv qacc)
  (if (or (null? f) (< (pdeg f) (pdeg g))) (cons (trim qacc) f)
    (let ((term (monomial (imod (* (lead-coef f p) ginv) p) (- (pdeg f) (pdeg g)))))
      (pdm (psub f (pmul term g p) p) g p ginv (padd qacc term p)))))
(define (pmod f g p) (cdr (pdivmod f g p)))
(define (pquot f g p) (car (pdivmod f g p)))

; ---------- gcd, derivative, modular exponentiation ----------
(define (pgcd a b p) (if (pzero? b) (if (pzero? a) '() (pmonic a p)) (pgcd b (pmod a b p) p)))
(define (pd l k) (if (null? l) '() (cons (* k (car l)) (pd (cdr l) (+ k 1)))))
(define (pderiv f p) (if (< (pdeg f) 1) '() (pclean (pd (cdr f) 1) p)))
(define (ppowmod base e m p)
  (cond ((= e 0) (pmod (list 1) m p))
        ((= (imod e 2) 0) (let ((h (ppowmod base (quotient e 2) m p))) (pmod (pmul h h p) m p)))
        (else (pmod (pmul base (ppowmod base (- e 1) m p) p) m p))))
(define (pthroot f p) (trim (ptr f p 0)))
(define (ptr f p i) (if (>= i (length f)) '() (cons (nthc f i) (ptr f p (+ i p)))))

; ---------- irreducibility test ----------
(define (xpk f p k) (xpk-go (list 0 1) f p k))            ; x^(p^k) mod f
(define (xpk-go cur f p k) (if (= k 0) cur (xpk-go (ppowmod cur p f p) f p (- k 1))))
(define (cfd f p n primes)
  (cond ((null? primes) #t)
        ((= (pdeg (pgcd f (psub (xpk f p (quotient n (car primes))) (list 0 1) p) p)) 0) (cfd f p n (cdr primes)))
        (else #f)))
(define (irreducible? f p)
  (let ((n (pdeg f)))
    (cond ((< n 1) #f) ((= n 1) #t)
          (else (and (equal? (xpk f p n) (pmod (list 0 1) f p)) (cfd f p n (map car (factor-int n))))))))

; ---------- distinct-degree factorization (monic squarefree input) ----------
(define (ddf f p) (ddf-go (pmonic f p) p 1 (list 0 1) '()))
(define (ddf-go fstar p i h acc)
  (if (< (pdeg fstar) (* 2 i))
      (if (> (pdeg fstar) 0) (reverse (cons (cons fstar (pdeg fstar)) acc)) (reverse acc))
      (let ((h2 (ppowmod h p fstar p)))
        (let ((g (pgcd fstar (psub h2 (list 0 1) p) p)))
          (if (> (pdeg g) 0)
              (let ((fs2 (pquot fstar g p))) (ddf-go fs2 p (+ i 1) (pmod h2 fs2 p) (cons (cons g i) acc)))
              (ddf-go fstar p (+ i 1) h2 acc))))))

; ---------- equal-degree factorization (block of degree-d irreducibles) ----------
(define (digits k p) (if (= k 0) '() (cons (imod k p) (digits (quotient k p) p))))
(define (cand k p) (trim (digits k p)))
(define (tm acc cur g d i) (if (>= i d) acc (let ((nx (pmod (pmul cur cur 2) g 2))) (tm (padd acc nx 2) nx g d (+ i 1)))))
(define (edf-elt a g d p) (if (= p 2) (tm a a g d 1) (psub (ppowmod a (quotient (- (expt p d) 1) 2) g p) (list 1) p)))
(define (edf g d p)
  (cond ((< (pdeg g) 1) '()) ((= (pdeg g) d) (list (pmonic g p))) (else (edf-try g d p 1))))
(define (edf-try g d p k)
  (let ((c (pgcd g (edf-elt (cand k p) g d p) p)))
    (if (and (> (pdeg c) 0) (< (pdeg c) (pdeg g)))
        (append (edf c d p) (edf (pquot g c p) d p))
        (edf-try g d p (+ k 1)))))
(define (cz-blocks blocks p) (if (null? blocks) '() (append (edf (car (car blocks)) (cdr (car blocks)) p) (cz-blocks (cdr blocks) p))))
(define (cz-squarefree s p) (cz-blocks (ddf s p) p))

; ---------- full factorization with multiplicities ----------
(define (divout rem q p e) (let ((dm (pdivmod rem q p))) (if (pzero? (cdr dm)) (divout (car dm) q p (+ e 1)) (cons rem e))))
(define (mvd rem irrs p acc)
  (cond ((null? irrs) (if (> (pdeg rem) 0) (append (reverse acc) (ff rem p)) (reverse acc)))
        (else (let ((res (divout rem (car irrs) p 0))) (mvd (car res) (cdr irrs) p (cons (cons (car irrs) (cdr res)) acc))))))
(define (ff f p)
  (cond ((< (pdeg f) 1) '())
        (else (let ((fp (pderiv f p)))
                (if (pzero? fp)
                    (map (lambda (qe) (cons (car qe) (* p (cdr qe)))) (ff (pthroot f p) p))
                    (mvd f (cz-squarefree (pquot f (pgcd f fp p) p) p) p '()))))))
(define (factor-mod f p) (ff (pmonic (pclean f p) p) p))

; ---------- certificates ----------
(define (ppow base e p) (if (= e 0) (list 1) (pmul base (ppow base (- e 1) p) p)))
(define (reconstruct fs p) (if (null? fs) (list 1) (pmul (ppow (car (car fs)) (cdr (car fs)) p) (reconstruct (cdr fs) p) p)))
(define (all-irred? fs p) (cond ((null? fs) #t) ((irreducible? (car (car fs)) p) (all-irred? (cdr fs) p)) (else #f)))
(define (factor-mod-ok? f p)
  (let ((fs (factor-mod f p)))
    (and (equal? (reconstruct fs p) (pmonic (pclean f p) p)) (all-irred? fs p))))

; ---------- display ----------
(define (term->string c i)
  (cond ((= i 0) (number->string c))
        ((= i 1) (if (= c 1) "x" (string-append (number->string c) "x")))
        (else (if (= c 1) (string-append "x^" (number->string i)) (string-append (number->string c) "x^" (number->string i))))))
(define (poly->string-go f i)
  (cond ((null? f) "")
        ((= (car f) 0) (poly->string-go (cdr f) (+ i 1)))
        ((null? (dropz (cdr f))) (term->string (car f) i))
        (else (string-append (term->string (car f) i) " + " (poly->string-go (cdr f) (+ i 1))))))
(define (poly->string f) (if (pzero? f) "0" (poly->string-go (trim f) 0)))
(define (factor->string fs)
  (cond ((null? fs) "1")
        ((null? (cdr fs)) (fac1 (car fs)))
        (else (string-append (fac1 (car fs)) " * " (factor->string (cdr fs))))))
(define (fac1 qe) (if (= (cdr qe) 1) (string-append "[" (poly->string (car qe)) "]") (string-append "[" (poly->string (car qe)) "]^" (number->string (cdr qe)))))
