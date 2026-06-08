; -*- lisp -*-
; lib/cas/bch.lisp -- binary BCH codes over GF(2^m).
;
; BCH codes are the cyclic-code generalization of Hamming codes.  Fix a primitive element
; alpha of GF(2^m); the code of length n = 2^m - 1 designed to correct t errors has generator
;   g(x) = lcm( M_1(x), M_2(x), ..., M_{2t}(x) ),
; where M_i is the minimal polynomial of alpha^i over GF(2).  Each M_i is the product over
; the cyclotomic coset {i, 2i, 4i, ...} (mod n) of (x - alpha^j); it has coefficients in
; GF(2), and distinct cosets give coprime minimal polynomials, so g is their product.  A
; k-bit message (k = n - deg g) is encoded as c = m * g over GF(2).
;
; Decoding: the syndromes S_j = r(alpha^j) live in GF(2^m); for <= t errors they obey a
; linear recurrence whose shortest generator -- found by Berlekamp-Massey over GF(2^m) -- is
; the error locator.  A Chien search finds the error positions as the i with
; Lambda(alpha^{-i}) = 0, and because the errors are binary (magnitude 1) decoding simply
; flips those bits, with no Forney step.  Certified against the textbook [15,7] BCH
; generator x^8 + x^7 + x^6 + x^4 + 1 and by the round trip.  Builds on gfp.lisp.

(import "cas/gfp.lisp")

; ---------- bit / list helpers ----------
(define (bxor a b) (if (= a b) 0 1))
(define (nth0 l i) (if (= i 0) (car l) (nth0 (cdr l) (- i 1))))
(define (set-nth l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (set-nth (cdr l) (- i 1) v))))
(define (padto l n) (if (>= (length l) n) l (append l (zeros (- n (length l))))))
(define (bch-member? x l) (cond ((null? l) #f) ((= x (car l)) #t) (else (bch-member? x (cdr l)))))
(define (bch-min l) (if (null? (cdr l)) (car l) (let ((r (bch-min (cdr l)))) (if (< (car l) r) (car l) r))))

; ---------- polynomials over GF(2^m): coeffs are field elements (bit lists), '() is zero ----------
(define (gp-at P i) (cond ((null? P) '()) ((= i 0) (car P)) (else (gp-at (cdr P) (- i 1)))))
(define (gp-strip P) (cond ((null? P) '()) ((gf-zero? (car P)) (gp-strip (cdr P))) (else P)))
(define (gp-reverse l) (gpr l '()))
(define (gpr l acc) (if (null? l) acc (gpr (cdr l) (cons (car l) acc))))
(define (gp-trim P) (gp-reverse (gp-strip (gp-reverse P))))
(define (gf-zeros k) (if (= k 0) '() (cons '() (gf-zeros (- k 1)))))
(define (gp-shift P k) (append (gf-zeros k) P))
(define (gpoly-add A B p) (cond ((null? A) B) ((null? B) A) (else (cons (gf-add (car A) (car B) p) (gpoly-add (cdr A) (cdr B) p)))))
(define (gpoly-scale B c m p) (map (lambda (x) (gf-mul c x m p)) B))
(define (gpoly-mul A B m p)
  (if (null? A) '()
      (gpoly-add (gpoly-scale B (car A) m p) (gp-shift (gpoly-mul (cdr A) B m p) 1) p)))
(define (gpoly-eval P pt m p) (gev P pt m p (gf-one) '()))
(define (gev P pt m p pti acc) (if (null? P) acc (gev (cdr P) pt m p (gf-mul pti pt m p) (gf-add acc (gf-mul (car P) pti m p) p))))

; ---------- code parameters ----------
(define (bch-n m) (- (expt 2 m) 1))
(define (bch-prim m p) (bp 1 (gf-modulus p m) p m))
(define (bp k mod p m) (let ((e (gf-elt k p m))) (if (and (not (gf-zero? e)) (gf-primitive? e mod p)) e (bp (+ k 1) mod p m))))

; ---------- cyclotomic cosets and minimal polynomials ----------
(define (coset i n) (coset-go (imod (* 2 i) n) i n (list i)))
(define (coset-go s i n acc) (if (= s i) acc (coset-go (imod (* 2 s) n) i n (cons s acc))))
(define (coset-leader i n) (bch-min (coset i n)))
(define (min-poly-bits i alpha mod p n) (extract-gf2 (mp (coset i n) alpha mod p (list (gf-one)))))
(define (mp cs alpha mod p acc) (if (null? cs) acc (mp (cdr cs) alpha mod p (gpoly-mul acc (list (gf-pow alpha (car cs) mod p) (gf-one)) mod p))))
(define (extract-gf2 gp) (map (lambda (c) (if (gf-zero? c) 0 1)) gp))

; ---------- generator g(x) = lcm of M_1..M_{2t}, over GF(2) ----------
(define (bch-generator alpha t mod p n) (bg 1 (* 2 t) alpha mod p n '() (list 1)))
(define (bg i hi alpha mod p n seen g)
  (if (> i hi) g
      (let ((lead (coset-leader i n)))
        (if (bch-member? lead seen) (bg (+ i 1) hi alpha mod p n seen g)
            (bg (+ i 1) hi alpha mod p n (cons lead seen) (pmul g (min-poly-bits i alpha mod p n) 2))))))
(define (bch-deg g) (- (length (trim g)) 1))
(define (bch-k n g) (- n (bch-deg g)))

; ---------- encode / corrupt (binary) ----------
(define (bch-encode msg g n) (padto (pnorm (pmul (pnorm msg 2) g 2) 2) n))
(define (bch-corrupt cw positions) (if (null? positions) cw (bch-corrupt (set-nth cw (car positions) (bxor (nth0 cw (car positions)) 1)) (cdr positions))))

; ---------- syndromes in GF(2^m) ----------
(define (gf-eval-bits r beta mod p) (geb r beta mod p (gf-one) '()))
(define (geb r beta mod p betai acc) (if (null? r) acc (geb (cdr r) beta mod p (gf-mul betai beta mod p) (if (= (imod (car r) p) 1) (gf-add acc betai p) acc))))
(define (bch-syndromes r alpha t mod p) (bs r alpha 1 (* 2 t) mod p))
(define (bs r alpha j hi mod p) (if (> j hi) '() (cons (gf-eval-bits r (gf-pow alpha j mod p) mod p) (bs r alpha (+ j 1) hi mod p))))
(define (bch-all-zero? S) (cond ((null? S) #t) ((gf-zero? (car S)) (bch-all-zero? (cdr S))) (else #f)))

; ---------- Berlekamp-Massey over GF(2^m) ----------
(define (bch-disc C S n L mod p) (bd C S n 1 L (gp-at S n) mod p))
(define (bd C S n i L acc mod p) (if (> i L) acc (bd C S n (+ i 1) L (gf-add acc (gf-mul (gp-at C i) (gp-at S (- n i)) mod p) p) mod p)))
(define (bch-bm S mod p) (bmg S mod p (length S) 0 (list (gf-one)) (list (gf-one)) 0 1 (gf-one)))
(define (bmg S mod p N n C B L mm b)
  (if (>= n N) (cons (gp-trim C) L)
      (let ((d (bch-disc C S n L mod p)))
        (if (gf-zero? d) (bmg S mod p N (+ n 1) C B L (+ mm 1) b)
            (let ((coeff (gf-mul d (gf-inv b mod p) mod p)))
              (let ((Cnew (gpoly-add C (gp-shift (gpoly-scale B coeff mod p) mm) p)))
                (if (<= (* 2 L) n)
                    (bmg S mod p N (+ n 1) Cnew C (+ (- n L) 1) 1 d)
                    (bmg S mod p N (+ n 1) Cnew B L (+ mm 1) b))))))))

; ---------- Chien search ----------
(define (bch-chien Lam alpha n mod p) (bc Lam alpha 0 n mod p))
(define (bc Lam alpha i n mod p)
  (cond ((>= i n) '())
        ((gf-zero? (gpoly-eval Lam (gf-pow alpha (- n i) mod p) mod p)) (cons i (bc Lam alpha (+ i 1) n mod p)))
        (else (bc Lam alpha (+ i 1) n mod p))))

; ---------- decode ----------
(define (bch-recover c g k) (let ((dm (pdivmod (trim c) (trim g) 2))) (if (null? (trim (cdr dm))) (padto (trim (car dm)) k) 'fail)))
(define (bch-decode r alpha t mod p n g)
  (let ((rr (padto r n)))
    (let ((S (bch-syndromes rr alpha t mod p)))
      (if (bch-all-zero? S) (bch-recover rr g (bch-k n g))
          (let ((bmr (bch-bm S mod p)))
            (let ((Lam (car bmr)) (L (cdr bmr)))
              (let ((pos (bch-chien Lam alpha n mod p)))
                (if (or (= (length pos) 0) (not (= (length pos) L))) 'fail
                    (bch-recover (bch-corrupt rr pos) g (bch-k n g))))))))))

; ---------- certificates ----------
(define (bits-eq? a b) (equal? (trim (pnorm a 2)) (trim (pnorm b 2))))
(define (bch-clean-ok? msg alpha t mod p n g) (bits-eq? (bch-decode (bch-encode msg g n) alpha t mod p n g) msg))
(define (bch-roundtrip-ok? msg alpha t mod p n g errs) (bits-eq? (bch-decode (bch-corrupt (bch-encode msg g n) errs) alpha t mod p n g) msg))

; ---------- display ----------
(define (bch-info m t g) (string-append "[" (number->string (bch-n m)) "," (number->string (bch-k (bch-n m) g)) "] binary BCH over GF(2^" (number->string m) "), corrects " (number->string t)))
