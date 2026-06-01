; -*- lisp -*-
; lib/cas/gosper.lisp — Gosper's algorithm: indefinite hypergeometric summation,
; the discrete analogue of the Risch decision procedure.
;
; A term t(n) is hypergeometric when r(n) = t(n+1)/t(n) is a rational function of
; n.  Gosper decides whether t has a hypergeometric antidifference S (i.e.
; S(n+1) - S(n) = t(n)) and, if so, produces a rational R(n) with S(n) = R(n) t(n)
; so that SUM_{k} t(k) telescopes to R(n) t(n).  If none exists it PROVES so.
;
; Method (Gosper / Petkovsek):
;   1. Gosper-Petkovsek normal form  r = (a/b) (c(n+1)/c(n)) with gcd(a(n),b(n+h))
;      = 1 for all integers h >= 0 (the shifts come from the non-negative integer
;      roots of resultant_n(a(n), b(n+h)), found by factoring that resultant).
;   2. Solve Gosper's equation  a(n) x(n+1) - b(n-1) x(n) = c(n)  for a polynomial
;      x with a degree bound; solvable iff t is Gosper-summable.  Then
;      R(n) = b(n-1) x(n) / c(n).
;
; CERTIFICATE (purely rational, no hypergeometric needed): S(n+1)-S(n)=t(n) holds
; iff  R(n+1) r(n) - R(n) = 1  as rational functions of n, which we check exactly.
;
; Top-level helpers only; builds on resultant.lisp (poly + resultant) and factor.lisp.

(import "cas/resultant.lisp")
(import "cas/factor.lisp")

(define (iota a b) (if (> a b) '() (cons a (iota (+ a 1) b))))

; p(n+k) by Horner with the linear polynomial (n+k)
(define (poly-shift p k) (pshift (reverse (poly-norm p)) (list k 1) '()))
(define (pshift rc nk acc) (if (null? rc) acc (pshift (cdr rc) nk (poly-add (poly-mul acc nk) (list (car rc))))))

; ============================================================
;  exact linear solver over Q  (Gauss-Jordan to RREF)
;  aug: list of rows, each row = (ncols coeffs ... rhs).  -> solution list | 'none
; ============================================================
(define (set-row rows i v) (set-index rows i v))
(define (swap-rows rows i j) (set-index (set-index rows i (nth rows j)) j (nth rows i)))
(define (find-nz rows col start)
  (cond ((>= start (length rows)) 'none) ((not (= (nth (nth rows start) col) 0)) start) (else (find-nz rows col (+ start 1)))))
(define (scale-row rows i f) (set-index rows i (vec-scale f (nth rows i))))
(define (elim-col rows col piv r)
  (cond ((>= r (length rows)) rows)
        ((= r piv) (elim-col rows col piv (+ r 1)))
        (else (let ((fac (nth (nth rows r) col)))
                (elim-col (if (= fac 0) rows (set-row rows r (vec-sub (nth rows r) (vec-scale fac (nth rows piv))))) col piv (+ r 1))))))
(define (rref-loop rows col prow ncols)
  (if (or (>= prow (length rows)) (>= col ncols)) rows
    (let ((pivi (find-nz rows col prow)))
      (if (equal? pivi 'none) (rref-loop rows (+ col 1) prow ncols)
        (let ((r2 (scale-row (swap-rows rows prow pivi) prow (/ 1 (nth (nth (swap-rows rows prow pivi) prow) col)))))
          (rref-loop (elim-col r2 col prow 0) (+ col 1) (+ prow 1) ncols))))))
(define (all-zero-row row ncols) (cond ((>= ncols 0) (az row ncols 0)) (else #t)))
(define (az row ncols k) (cond ((>= k ncols) #t) ((not (= (nth row k) 0)) #f) (else (az row ncols (+ k 1)))))
(define (inconsistent? rows ncols)
  (cond ((null? rows) #f)
        ((and (az (car rows) ncols 0) (not (= (nth (car rows) ncols) 0))) #t)
        (else (inconsistent? (cdr rows) ncols))))
(define (leading-col row ncols k) (cond ((>= k ncols) ncols) ((not (= (nth row k) 0)) k) (else (leading-col row ncols (+ k 1)))))
(define (pivot-row-for rows ncols j i)
  (cond ((>= i (length rows)) 'none) ((= (leading-col (nth rows i) ncols 0) j) i) (else (pivot-row-for rows ncols j (+ i 1)))))
(define (lin-solve aug ncols)
  (let ((e (rref-loop aug 0 0 ncols)))
    (if (inconsistent? e ncols) 'none
      (map (lambda (j) (let ((ri (pivot-row-for e ncols j 0))) (if (equal? ri 'none) 0 (nth (nth e ri) ncols)))) (iota 0 (- ncols 1))))))

; ============================================================
;  Gosper-Petkovsek normal form
; ============================================================
(define (maxdeg ps) (md ps -1))
(define (md ps m) (if (null? ps) m (md (cdr ps) (max m (poly-deg (car ps))))))

(define (gosper-res-poly f q)                ; R(h) = res_n(f(n), q(n+h)) as a poly in h
  (let ((bound (max (* (poly-deg f) (poly-deg q)) 1)))
    (lagrange (gres-pts f q 0 bound))))
(define (gres-pts f q h count) (if (> h count) '() (cons (cons h (resultant f (poly-shift q h))) (gres-pts f q (+ h 1) count))))

(define (nonneg-int-roots R)                 ; from linear factors of R(h)
  (if (poly-zero? R) '() (sort-asc (filter-nni (lin-roots (car (cdr (factor-Q R))))))))
(define (lin-roots facs)
  (if (null? facs) '()
    (let ((f (car (cdr (car facs)))))
      (if (= (poly-deg f) 1) (cons (/ (- 0 (poly-coeff f 0)) (poly-coeff f 1)) (lin-roots (cdr facs))) (lin-roots (cdr facs))))))
(define (filter-nni xs) (cond ((null? xs) '()) ((and (integer? (car xs)) (>= (car xs) 0)) (cons (car xs) (filter-nni (cdr xs)))) (else (filter-nni (cdr xs)))))
(define (sort-asc xs) (if (null? xs) '() (let ((m (minimum xs))) (cons m (sort-asc (remove-one xs m))))))
(define (minimum xs) (if (null? (cdr xs)) (car xs) (min (car xs) (minimum (cdr xs)))))
(define (remove-one xs v) (cond ((null? xs) '()) ((= (car xs) v) (cdr xs)) (else (cons (car xs) (remove-one (cdr xs) v)))))

(define (prod-shifts s j h) (if (> j h) (list 1) (poly-mul (poly-shift s (- 0 j)) (prod-shifts s (+ j 1) h))))
(define (gp-loop a b c hs)
  (if (null? hs) (list a b c)
    (let ((h (car hs)))
      (let ((s (poly-gcd a (poly-shift b h))))
        (gp-loop (poly-div a s) (poly-div b (poly-shift s (- 0 h))) (poly-mul c (prod-shifts s 1 h)) (cdr hs))))))
(define (gosper-form rnum rden)
  (let ((g0 (poly-gcd rnum rden)))
    (let ((f (poly-div rnum g0)) (q (poly-div rden g0)))
      (gp-loop f q (list 1) (nonneg-int-roots (gosper-res-poly f q))))))

; ============================================================
;  Gosper's equation:  a(n) x(n+1) - B(n) x(n) = c(n),  B(n) = b(n-1)
; ============================================================
(define (gosper-degree-bound A B c)
  (let ((dA (poly-deg A)) (dB (poly-deg B)) (dc (poly-deg c)))
    (cond ((not (= dA dB)) (- dc (max dA dB)))
          ((not (= (poly-lead A) (poly-lead B))) (- dc dA))
          (else (let ((spec (/ (- (poly-coeff B (- dA 1)) (poly-coeff A (- dA 1))) (poly-lead A))))
                  (if (and (integer? spec) (>= spec 0)) (max spec (+ (- dc dA) 1)) (+ (- dc dA) 1)))))))

(define (gosper-solve-deg A B c d)
  (let ((Ls (map (lambda (i) (poly-sub (poly-mul A (poly-shift (poly-monomial 1 i) 1)) (poly-mul B (poly-monomial 1 i)))) (iota 0 d))))
    (let ((E (+ 1 (max (maxdeg (cons c Ls)) 0))))
      (let ((aug (map (lambda (j) (append (map (lambda (Li) (poly-coeff Li j)) Ls) (list (poly-coeff c j)))) (iota 0 (- E 1)))))
        (let ((sol (lin-solve aug (+ d 1))))
          (if (equal? sol 'none) 'none (poly-norm sol)))))))

; ============================================================
;  top level
; ============================================================
; -> (list 'summable Rnum Rden)  with S(n) = (Rnum/Rden) t(n),  or  (list 'not-summable)
(define (gosper-sum rnum rden)
  (let ((gp (gosper-form rnum rden)))
    (let ((a (car gp)) (b (car (cdr gp))) (c (car (cdr (cdr gp)))))
      (let ((B (poly-shift b -1)))
        (let ((d (gosper-degree-bound a B c)))
          (if (< d 0) (list 'not-summable)
            (let ((x (gosper-solve-deg a B c d)))
              (if (equal? x 'none) (list 'not-summable)
                (let ((rn (poly-mul B x)))
                  (let ((g (poly-gcd rn c)))
                    (list 'summable (poly-div rn g) (poly-div c g))))))))))))

; certificate: R(n+1) r(n) - R(n) = 1  as rational functions
(define (gosper-certificate rnum rden rn rd)
  (equal? (poly-norm (poly-sub (poly-mul (poly-mul (poly-shift rn 1) rnum) rd)
                               (poly-mul rn (poly-mul (poly-shift rd 1) rden))))
          (poly-norm (poly-mul (poly-shift rd 1) (poly-mul rden rd)))))

(define (gosper-result->string res)
  (if (equal? (car res) 'not-summable) "no hypergeometric antidifference (proved)"
    (string-append "S(n) = ( " (poly->string (car (cdr res)) "n")
                   (if (equal? (car (cdr (cdr res))) (list 1)) "" (string-append " ) / ( " (poly->string (car (cdr (cdr res))) "n")))
                   " ) * t(n)")))
