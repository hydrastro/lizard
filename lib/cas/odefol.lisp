; -*- lisp -*-
; lib/cas/odefol.lisp -- closed-form POLYNOMIAL solutions of the first-order LINEAR ODE  y' + p(x) y = q(x)
; with polynomial coefficients p and q (docs/CAS.md -- summit S5, the variable-coefficient first-order linear
; case sitting between ode1's separable equations and odelin's constant-coefficient equations).
;
; The operator L(y) = y' + p y is LINEAR in the unknown coefficients of a polynomial ansatz
; P = c_0 + c_1 x + ... + c_m x^m.  When deg p >= 1 the top degree of L(P) is deg p + m, so a polynomial solution,
; if it exists, has degree m = deg q - deg p; we build the (deg q + 1) x (m + 1) matrix whose i-th column is the
; coefficient vector of L(x^i), and solve the exact linear system L(P) = q over Q with the Gauss-Jordan solver in
; linalg.lisp.  The candidate is then CERTIFIED by differentiation: P' + p P - q must be identically zero.  If the
; linear system is inconsistent (no polynomial solution -- the true solution needs the integrating factor exp INT p
; and is not polynomial) the module returns an honest 'no-polynomial-solution rather than guessing.
;
; This is the exact, certifiable polynomial slice of the integrating-factor method: sound both ways (a returned
; solution is verified; absence is reported, never fabricated), and genuinely more general than the
; constant-coefficient solver since p may be any polynomial of degree >= 1.
;
; Public (p, q polynomial coefficient lists low->high; deg p >= 1):
;   fol-particular p q     -> the polynomial solution P with P' + p P = q, or 'no-polynomial-solution
;   fol-certify p q P      -> #t iff P' + p P - q is identically zero (the differentiation certificate)
;   fol-solve p q          -> (list 'polynomial-solution P) certified | (list 'no-polynomial-solution)
;
; Verified: y' + y = x gives P = x - 1; y' + x y = x gives P = 1; y' + 2x y = 2x^3 gives P = x^2 - 1; an
; inconsistent case (no polynomial solution) is reported honestly; every returned P passes the differentiation
; certificate.
;
; Builds on poly.lisp and linalg.lisp.

(import "cas/poly.lisp")
(import "cas/linalg.lisp")

(define (fol-len l) (if (null? l) 0 (+ 1 (fol-len (cdr l)))))
(define (fol-nth l k) (if (= k 0) (car l) (fol-nth (cdr l) (- k 1))))
(define (fol-app a b) (if (null? a) b (cons (car a) (fol-app (cdr a) b))))

; ----- degree on a trimmed coefficient list -----
(define (fol-trim p) (fol-trim-go p (fol-len p)))
(define (fol-trim-go p n) (cond ((= n 0) 0) ((= (fol-nth p (- n 1)) 0) (fol-trim-go p (- n 1))) (else n)))
(define (fol-deg p) (- (fol-trim p) 1))

; ----- L applied to the monomial x^i: (x^i)' + p*x^i = i x^{i-1} + p x^i, returned as a coeff list -----
(define (fol-L-monomial p i) (poly-add (fol-deriv-monomial i) (poly-mul p (fol-monomial i))))
(define (fol-monomial i) (fol-app (fol-zeros i) (list 1)))
(define (fol-zeros k) (if (<= k 0) (quote ()) (cons 0 (fol-zeros (- k 1)))))
(define (fol-deriv-monomial i) (if (<= i 0) (list 0) (fol-app (fol-zeros (- i 1)) (list i))))

; ----- pad a coeff list to length n (low->high) -----
(define (fol-pad p n) (if (>= (fol-len p) n) p (fol-pad (fol-app p (list 0)) n)))

; ----- build the system matrix: rows indexed by power 0..R-1 (R = deg q + 1), columns by ansatz degree 0..m -----
(define (fol-rhs-len q) (+ (fol-deg q) 1))
(define (fol-ansatz-deg p q) (- (fol-deg q) (fol-deg p)))     ; m = deg q - deg p  (deg p >= 1)
; column i = coeff vector of L(x^i), padded to length R; the matrix is the list of ROWS, so transpose the columns.
(define (fol-columns p q) (fol-cols-go p (fol-rhs-len q) 0 (fol-ansatz-deg p q)))
(define (fol-cols-go p R i m) (if (> i m) (quote ()) (cons (fol-pad (fol-L-monomial p i) R) (fol-cols-go p R (+ i 1) m))))
; transpose a list of columns (each a length-R list) into R rows
(define (fol-transpose cols R) (fol-tr cols R 0))
(define (fol-tr cols R r) (if (>= r R) (quote ()) (cons (fol-row cols r) (fol-tr cols R (+ r 1)))))
(define (fol-row cols r) (if (null? cols) (quote ()) (cons (fol-nth (car cols) r) (fol-row (cdr cols) r))))

; ----- solve the linear system for the ansatz coefficients -----
(define (fol-particular p q) (fol-dispatch p q (fol-ansatz-deg p q)))
(define (fol-dispatch p q m) (if (< m 0) (quote no-polynomial-solution) (fol-from-solution p q (mat-solve (fol-transpose (fol-columns p q) (fol-rhs-len q)) (fol-pad q (fol-rhs-len q))))))
(define (fol-from-solution p q sol) (if (equal? sol (quote none)) (quote no-polynomial-solution) (fol-verify-or-fail p q sol)))
(define (fol-verify-or-fail p q P) (if (fol-certify p q P) P (quote no-polynomial-solution)))

; ----- the differentiation certificate: P' + p P - q identically zero -----
(define (fol-certify p q P) (poly-zero? (poly-sub (poly-add (poly-deriv P) (poly-mul p P)) q)))

; ----- the public solve wrapper -----
(define (fol-solve p q) (fol-wrap (fol-particular p q)))
(define (fol-wrap P) (if (equal? P (quote no-polynomial-solution)) (list (quote no-polynomial-solution)) (list (quote polynomial-solution) P)))
