; -*- lisp -*-
; lib/cas/pfd.lisp -- partial fraction decomposition over Q.
;
; For p/q, first divide out the polynomial part so deg r < deg q, then factor q over Q
; into irreducible powers q = prod qi^ei.  The decomposition
;
;     p/q  =  s(x)  +  sum_i sum_{j=1..ei}  A_ij(x) / qi(x)^j ,   deg A_ij < deg qi
;
; has exactly deg q unknown coefficients.  Clearing denominators turns the identity into
; r = sum A_ij * (q / qi^j); matching the deg q coefficients of x gives a square rational
; linear system, solved with the Gauss-Jordan solver from gosper.lisp.  The result is
; checked by recombination: s*q + sum A_ij*(q/qi^j) must equal p exactly, so a wrong
; decomposition is never returned.  Irreducible quadratics stay intact, so the form is a
; genuine real/rational partial fraction (no complex numbers introduced).
;
; Builds on factor.lisp (factorization over Q), poly.lisp, and gosper.lisp.

(import "cas/factor.lisp")
(import "cas/gosper.lisp")

(define (cidx p i) (if (< i (length p)) (nth p i) 0))
(define (zeros k) (if (= k 0) '() (cons 0 (zeros (- k 1)))))
(define (xpow d) (append (zeros d) (list 1)))
(define (take-n l n) (if (= n 0) '() (cons (car l) (take-n (cdr l) (- n 1)))))
(define (drop-n l n) (if (= n 0) l (drop-n (cdr l) (- n 1))))

; q / qi^j  (exact; qi^j divides q)
(define (q-over q qi j) (car (poly-divmod q (poly-pow qi j))))

; build a unit list: each unit is (list qi j base nd), base = q/qi^j, nd = deg qi
(define (build-units q fl) (if (null? fl) '() (append (fuf q (car (cdr (car fl))) 1 (car (car fl)) (poly-deg (car (cdr (car fl))))) (build-units q (cdr fl)))))
(define (fuf q qi j mult nd) (if (> j mult) '() (cons (list qi j (q-over q qi j) nd) (fuf q qi (+ j 1) mult nd))))

; the nd basis polynomials of a unit: x^d * base, d = 0..nd-1
(define (unit-basis u) (map (lambda (d) (poly-mul (xpow d) (car (cdr (cdr u))))) (iota 0 (- (car (cdr (cdr (cdr u)))) 1))))
(define (all-basis units) (if (null? units) '() (append (unit-basis (car units)) (all-basis (cdr units)))))

; coefficient-matching system: row k is the x^k equation, augmented with r_k
(define (pfd-rows basis r dq) (map (lambda (k) (append (map (lambda (b) (cidx b k)) basis) (list (cidx r k)))) (iota 0 (- dq 1))))

; slice the solution vector back into A_ij per unit
(define (slice-units units c)
  (if (null? units) '()
    (cons (list (car (car units)) (car (cdr (car units))) (poly-norm (take-n c (car (cdr (cdr (cdr (car units))))))))
          (slice-units (cdr units) (drop-n c (car (cdr (cdr (cdr (car units))))))))))

; p/q -> (list s terms), terms = list of (qi j A_ij); 'singular only if the solve fails
(define (partial-fractions p q)
  (let ((sr (poly-divmod p q)))
    (let ((units (build-units q (car (cdr (factor-Q q))))))
      (let ((c (lin-solve (pfd-rows (all-basis units) (car (cdr sr)) (poly-deg q)) (poly-deg q))))
        (if (equal? c 'none) 'singular (list (car sr) (slice-units units c)))))))

; ---------- certificate ----------
(define (pfd-recon q terms) (if (null? terms) '() (poly-add (poly-mul (car (cdr (cdr (car terms)))) (q-over q (car (car terms)) (car (cdr (car terms))))) (pfd-recon q (cdr terms)))))
(define (pfd-ok? p q ans) (and (not (equal? ans 'singular)) (equal? (poly-norm p) (poly-norm (poly-add (poly-mul (car ans) q) (pfd-recon q (car (cdr ans))))))))

; ---------- display ----------
(define (paren s) (string-append "[" s "]"))
(define (term->string t var)
  (string-append (paren (poly->string (car (cdr (cdr t))) var)) " / " (paren (poly->string (car t) var)) (if (= (car (cdr t)) 1) "" (string-append "^" (number->string (car (cdr t)))))))
(define (nonzero-term? t) (not (poly-zero? (car (cdr (cdr t))))))
(define (terms->string ts var) (if (null? ts) "" (tg (filter nonzero-term? ts) var)))
(define (tg ts var) (cond ((null? ts) "0") ((null? (cdr ts)) (term->string (car ts) var)) (else (string-append (term->string (car ts) var) " + " (tg (cdr ts) var)))))
(define (pfd->string ans var)
  (if (equal? ans 'singular) "singular"
    (let ((sp (if (poly-zero? (car ans)) "" (poly->string (car ans) var))) (tp (terms->string (car (cdr ans)) var)))
      (cond ((equal? sp "") tp) ((equal? tp "") sp) (else (string-append sp " + " tp))))))
