; -*- lisp -*-
; lib/cas/stirling.lisp -- Stirling numbers (both kinds) and Bell numbers.
;
; The Stirling numbers of the second kind S(n,k) count partitions of n elements into k
; nonempty blocks and satisfy S(n,k) = k S(n-1,k) + S(n-1,k-1).  The unsigned Stirling
; numbers of the first kind c(n,k) count permutations of n with k cycles and satisfy
; c(n,k) = (n-1) c(n-1,k) + c(n-1,k-1).  Both are built row by row, like Pascal's
; triangle, so there is no exponential recomputation.  The Bell number B(n) is the total
; number of partitions, B(n) = sum_k S(n,k).
;
; The numbers are certified by exact polynomial identities, not just spot values:
;   * x^n = sum_k S(n,k) * x^{falling k}   (second kind connects monomials to the
;     falling-factorial basis x(x-1)...(x-k+1));
;   * x(x+1)...(x+n-1) = sum_k c(n,k) x^k   (unsigned first kind are the coefficients of
;     the rising factorial);
;   * sum_k c(n,k) = n!   (every permutation has some number of cycles);
;   * B(n) computed by summing the second-kind row equals B(n) from the independent Bell
;     recurrence B(n) = sum_k C(n-1,k) B(k).
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (pnth l i) (if (= i 0) (car l) (pnth (cdr l) (- i 1))))
(define (prange a b) (if (> a b) '() (cons a (prange (+ a 1) b))))
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (lst-get l i) (if (or (< i 0) (>= i (length l))) 0 (pnth l i)))
(define (sumlist l) (if (null? l) 0 (+ (car l) (sumlist (cdr l)))))
(define (factorial n) (if (= n 0) 1 (* n (factorial (- n 1)))))
(define (binom n k) (/ (factorial n) (* (factorial k) (factorial (- n k)))))
(define (lastrow rows) (pnth rows (- (length rows) 1)))

; ---------- Stirling second kind ----------
(define (s2-row n prev) (map (lambda (k) (+ (* k (lst-get prev k)) (lst-get prev (- k 1)))) (prange 0 n)))
(define (s2t n N acc) (if (>= n N) acc (s2t (+ n 1) N (append acc (list (s2-row (+ n 1) (lastrow acc)))))))
(define (s2-table N) (s2t 0 N (list (list 1))))
(define (stirling2-row n) (lastrow (s2-table n)))
(define (stirling2 n k) (lst-get (stirling2-row n) k))

; ---------- Stirling first kind (unsigned) ----------
(define (s1-row n prev) (map (lambda (k) (+ (* (- n 1) (lst-get prev k)) (lst-get prev (- k 1)))) (prange 0 n)))
(define (s1t n N acc) (if (>= n N) acc (s1t (+ n 1) N (append acc (list (s1-row (+ n 1) (lastrow acc)))))))
(define (s1-table N) (s1t 0 N (list (list 1))))
(define (stirling1-row n) (lastrow (s1-table n)))
(define (stirling1 n k) (lst-get (stirling1-row n) k))

; ---------- Bell numbers ----------
(define (bell n) (sumlist (stirling2-row n)))
(define (bsum2 nm1 prev k acc) (if (> k nm1) acc (bsum2 nm1 prev (+ k 1) (+ acc (* (binom nm1 k) (pnth prev k))))))
(define (bell-step m prev) (bsum2 (- m 1) prev 0 0))
(define (bell-build m N acc) (if (> m N) acc (bell-build (+ m 1) N (append acc (list (if (= m 0) 1 (bell-step m acc)))))))
(define (bell-rec n) (pnth (bell-build 0 n '()) n))

; ---------- falling / rising factorial polynomials ----------
(define (fp i k acc) (if (>= i k) acc (fp (+ i 1) k (poly-mul acc (list (- 0 i) 1)))))
(define (falling-poly k) (fp 0 k (list 1)))
(define (rp i k acc) (if (>= i k) acc (rp (+ i 1) k (poly-mul acc (list i 1)))))
(define (rising-poly k) (rp 0 k (list 1)))

; ---------- certificates ----------
(define (s2id row k acc) (if (>= k (length row)) acc (s2id row (+ k 1) (poly-add acc (poly-scale (pnth row k) (falling-poly k))))))
(define (s2-identity-poly n) (s2id (stirling2-row n) 0 '()))
(define (xn-poly n) (append (zeros n) (list 1)))
(define (stirling2-ok? n) (equal? (poly-norm (s2-identity-poly n)) (poly-norm (xn-poly n))))
(define (stirling1-ok? n) (equal? (poly-norm (rising-poly n)) (poly-norm (stirling1-row n))))
(define (stirling1-sum-ok? n) (= (sumlist (stirling1-row n)) (factorial n)))
(define (bell-ok? n) (= (bell n) (bell-rec n)))

; ---------- display ----------
(define (bell-list-str N) (map bell (prange 0 N)))
