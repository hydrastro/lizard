; -*- lisp -*-
; lib/cas/elliptic3split.lisp -- SPLIT the norm N = A^2 - B^2 q to recover the third-kind element g = A + B*y on
; y^2 = q, completing for the constant-B case the construction whose first step (reconstructing N) was elliptic3norm
; (docs/CAS.md -- summit S1, turning the norm into the actual A and B).
;
; The norm equation A^2 - B^2 q = N is a norm form in the quadratic ring Q[x][y]/(y^2 - q); solving it in full
; generality is the Pell-type / Jacobian-torsion problem and stays open.  This module solves the SOUND, decidable
; slice B = constant c: then A^2 = N + c^2 q must be a PERFECT SQUARE polynomial, and A is its exact polynomial
; square root.  Polynomial square-rootedness is decided exactly -- the leading coefficient must be a rational
; square and the coefficient-matching square root (built top-down, the unique S with S^2 = P) must close with no
; remainder -- so the construction is two-sided: it returns g = A + c*y when N + c^2 q is a perfect square, and
; reports 'no-split otherwise.  The recovered g is then CERTIFIED by recomputing A^2 - B^2 q and checking it equals
; N exactly.  Everything is exact over Q; no approximation and no guessed g.
;
; Public (N, q polynomial coefficient lists low->high; c a rational constant for the B = c attempt):
;   esp-poly-sqrt P            -> the polynomial square root S with S^2 = P, or 'not-square
;   esp-is-square? P           -> #t iff P is a perfect square polynomial
;   esp-split-const N q c      -> the pair (A . c) with A^2 - c^2 q = N (B = c), or 'no-split
;   esp-verify N q A B         -> #t iff A^2 - B^2 q = N exactly (the norm certificate)
;   esp-recover-g N q c        -> (list 'g A c) meaning g = A + c*y, certified, or (list 'no-split)
;
; Verified: with q = x^2 + 1 and the norm N = -1, B = 1 gives N + q = x^2, a perfect square, so A = x and
; g = x + y (certified A^2 - B^2 q = x^2 - (x^2+1) = -1 = N); (x+1)^2 and 4x^2+4x+1 are recognized as squares with
; exact roots; x^2 + 1 is correctly rejected as not a square; a wrong split fails the certificate.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (esp-len l) (if (null? l) 0 (+ 1 (esp-len (cdr l)))))
(define (esp-nth l k) (if (= k 0) (car l) (esp-nth (cdr l) (- k 1))))
(define (esp-app a b) (if (null? a) b (cons (car a) (esp-app (cdr a) b))))

; ----- trim trailing zeros; degree -----
(define (esp-trim p) (esp-take p (esp-trim-n p (esp-len p))))
(define (esp-trim-n p n) (cond ((= n 0) 0) ((= (esp-nth p (- n 1)) 0) (esp-trim-n p (- n 1))) (else n)))
(define (esp-take p n) (if (= n 0) (quote ()) (esp-app (esp-take p (- n 1)) (list (esp-nth p (- n 1))))))

; ----- integer/rational perfect-square root or #f -----
(define (esp-int-sqrt n) (if (< n 0) #f (esp-isq n 0)))
(define (esp-isq n k) (cond ((> (* k k) n) #f) ((= (* k k) n) k) (else (esp-isq n (+ k 1)))))
(define (esp-rat-sqrt c) (if (< c 0) #f (esp-rat-sqrt-go (esp-int-sqrt (numerator c)) (esp-int-sqrt (denominator c)))))
(define (esp-rat-sqrt-go ns ds) (if ns (if ds (/ ns ds) #f) #f))

; ----- polynomial square root by top-down coefficient matching -----
(define (esp-poly-sqrt P) (esp-ps-dispatch (esp-trim P)))
(define (esp-ps-dispatch P)
  (cond ((null? P) (list 0))
        ((= (remainder (- (esp-len P) 1) 2) 1) (quote not-square))         ; odd degree -> not a square
        (else (esp-ps-build P (quotient (- (esp-len P) 1) 2) (esp-rat-sqrt (esp-nth P (- (esp-len P) 1)))))))
(define (esp-ps-build P m slead) (if slead (esp-ps-verify P (esp-ps-coeffs P m slead)) (quote not-square)))
; build S of degree m with leading coeff slead by matching coefficients of S^2 from the top down
(define (esp-ps-coeffs P m slead) (esp-ps-fill P m slead (esp-init-S m slead)))
(define (esp-init-S m slead) (esp-app (esp-zeros m) (list slead)))
(define (esp-zeros k) (if (<= k 0) (quote ()) (cons 0 (esp-zeros (- k 1)))))
(define (esp-ps-fill P m slead S) (esp-ps-loop P m slead S (- m 1)))
(define (esp-ps-loop P m slead S k)
  (if (< k 0) S
      (esp-ps-loop P m slead (esp-set S k (esp-solve-coeff P m slead S k)) (- k 1))))
; coefficient of x^(m+k) in S^2 is sum_{i+j=m+k} S_i S_j; isolate 2*slead*S_k
(define (esp-solve-coeff P m slead S k) (/ (- (esp-coeff P (+ m k)) (esp-conv-excl S (+ m k) m k)) (* 2 slead)))
(define (esp-coeff P i) (if (< i (esp-len P)) (esp-nth P i) 0))
; sum of S_i S_j over i+j = idx, EXCLUDING the (m,k) and (k,m) cross terms
(define (esp-conv-excl S idx m k) (esp-conv-go S idx m k 0 0))
(define (esp-conv-go S idx m k i acc)
  (cond ((> i (esp-len-1 S)) acc)
        ((esp-excluded? i (- idx i) m k) (esp-conv-go S idx m k (+ i 1) acc))
        ((esp-valid-j? (- idx i) S) (esp-conv-go S idx m k (+ i 1) (+ acc (* (esp-Sget S i) (esp-Sget S (- idx i))))))
        (else (esp-conv-go S idx m k (+ i 1) acc))))
(define (esp-len-1 S) (- (esp-len S) 1))
(define (esp-valid-j? j S) (if (< j 0) #f (if (> j (esp-len-1 S)) #f #t)))
(define (esp-excluded? i j m k) (if (if (= i m) (= j k) #f) #t (if (= i k) (= j m) #f)))
(define (esp-Sget S i) (if (esp-valid-j? i S) (esp-nth S i) 0))
(define (esp-set S k v) (esp-set-go S k v 0))
(define (esp-set-go S k v i) (if (null? S) (quote ()) (cons (if (= i k) v (car S)) (esp-set-go (cdr S) k v (+ i 1)))))
; verify S^2 = P exactly, else not-square
(define (esp-ps-verify P S) (if (esp-poly-eq? (poly-mul S S) P) S (quote not-square)))
(define (esp-poly-eq? a b) (equal? (esp-trim a) (esp-trim b)))

(define (esp-is-square? P) (if (equal? (esp-poly-sqrt P) (quote not-square)) #f #t))

; ----- split N = A^2 - c^2 q with B = c constant: A = sqrt(N + c^2 q) -----
(define (esp-split-const N q c) (esp-sc-dispatch N q c (esp-poly-sqrt (poly-add N (poly-scale (* c c) q)))))
(define (esp-sc-dispatch N q c A) (if (equal? A (quote not-square)) (quote no-split) (cons A c)))

; ----- certificate: A^2 - B^2 q = N  (B is a scalar constant, so B^2 q = poly-scale (B*B) q) -----
(define (esp-verify N q A B) (esp-poly-eq? (poly-sub (poly-mul A A) (poly-scale (* B B) q)) N))

; ----- recover g = A + c*y, certified -----
(define (esp-recover-g N q c) (esp-rg-finish N q c (esp-split-const N q c)))
(define (esp-rg-finish N q c split) (if (equal? split (quote no-split)) (list (quote no-split)) (esp-rg-cert N q (car split) c)))
(define (esp-rg-cert N q A c) (if (esp-verify N q A c) (list (quote g) A c) (list (quote no-split))))
