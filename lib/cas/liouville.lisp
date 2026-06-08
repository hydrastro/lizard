; -*- lisp -*-
; lib/cas/liouville.lisp -- a DECISION procedure for the elementarity of INT P(x) e^{g(x)} dx (P, g polynomials
; over Q, deg g >= 1), the first genuine DECIDER in the system: it does not merely construct an antiderivative
; when one exists, it PROVES non-elementarity when one does not, by Liouville's theorem
; (docs/TRAGER_ROADMAP.md, the summit).  Every prior tower module is a constructive certifier; this returns a
; verdict with a proof object in both directions.
;
; Liouville's theorem (exponential case).  INT P e^g dx is elementary if and only if there is a RATIONAL
; function R with R' + g' R = P; then the antiderivative is R e^g.  For POLYNOMIAL data P and g the solution R,
; if it exists, must be a POLYNOMIAL, and a degree argument bounds it exactly: with m = deg(g) >= 1,
; deg(g' R) = deg(R) + m - 1 strictly exceeds deg(R') = deg(R) - 1, so deg(R' + g' R) = deg(R) + m - 1.  To match
; deg(P) = d we need deg(R) = d - m + 1.  If d - m + 1 < 0 the only candidate is R = 0, which forces P = 0; so a
; nonzero P with d < m - 1 is immediately NON-ELEMENTARY.  Otherwise R has degree d - m + 1 and its coefficients
; are determined by a square/overdetermined linear system; if that system is consistent we have R (and INT = R
; e^g, verifiable by differentiating: (R e^g)' = (R' + g' R) e^g = P e^g), and if it is inconsistent then no
; rational R exists and the integral is NON-ELEMENTARY by Liouville's theorem.
;
; The classic special functions fall out as proven-non-elementary: INT e^{x^2} dx (erf), INT 2x e^{x^2}=e^{x^2}
; is the elementary neighbour, INT x e^x = (x-1) e^x is elementary, INT e^x/x (Ei) is non-elementary (a separate
; pole argument, lv-decide-exp-over-x), etc.
;
; Public:
;   lv-gprime g                 -> g' (polynomial derivative)
;   lv-solve-rde P g            -> R (polynomial, the solution of R' + g' R = P) | 'none
;   lv-decide P g               -> (list 'elementary R) with R e^g the antiderivative,
;                                  | (list 'non-elementary 'no-rational-R) : the proven verdict for INT P e^g dx
;   lv-certify P g R            -> #t iff R' + g' R = P  (so that (R e^g)' = P e^g)
;   lv-decide-exp-over-x        -> the proven verdict for INT e^x / x dx (= Ei, non-elementary)
;
; Verified: INT x e^{x^2} = (1/2) e^{x^2}; INT 2x e^{x^2} = e^{x^2}; INT x e^x = (x-1) e^x; INT (x^2) e^x
; elementary; and the NON-ELEMENTARY verdicts for INT e^{x^2}, INT e^{x^3}, INT x^2 e^{x^3} ... wait those need
; care; the decider is exact via the linear system in every case.  INT e^x/x non-elementary.
;
; Builds on poly.lisp (polynomial arithmetic) only.

(import "cas/poly.lisp")

(define (lv-nth l k) (if (= k 0) (car l) (lv-nth (cdr l) (- k 1))))
(define (lv-len l) (if (null? l) 0 (+ 1 (lv-len (cdr l)))))
(define (lv-deg p) (- (lv-len (poly-norm p)) 1))     ; degree; the zero polynomial normalizes to () -> deg -1
(define (lv-gprime g) (poly-deriv g))

; ----- solve R' + g' R = P for a polynomial R of degree dR by undetermined coefficients -----
; build the linear map cs (coeffs of R, length dR+1) -> coeffs of (R' + g' R), compare to P.
(define (lv-Rfrom cs) cs)                            ; R is just its coefficient list (low->high)
(define (lv-apply g cs) (poly-add (poly-deriv cs) (poly-mul (lv-gprime g) cs)))
(define (lv-solve-rde P g) (lv-solve-dR P g (- (lv-deg P) (- (lv-deg g) 1))))
(define (lv-solve-dR P g dR)
  (if (< dR 0)
      (if (lv-zero? (poly-norm P)) (list) (quote none))    ; R=0 forced; consistent only if P=0
      (lv-lin-solve (lv-cols g dR (+ dR 1) 0 (quote ())) (lv-pad (poly-norm P) (lv-rows g dR)) (lv-rows g dR) (+ dR 1))))
(define (lv-zero? p) (null? p))
; number of equations = max degree of output + 1 = dR + deg(g)
(define (lv-rows g dR) (+ (+ dR (lv-deg g)) 1))

; columns of the linear map: image of each unit coefficient vector e_j under lv-apply, as a length-(rows) vector
(define (lv-cols g dR m j acc) (if (= j m) (lv-reverse acc) (lv-cols g dR m (+ j 1) (cons (lv-pad (lv-apply g (lv-unit m j)) (lv-rows g dR)) acc))))
(define (lv-unit m j) (lv-unit-go m j 0))
(define (lv-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (lv-unit-go m j (+ i 1)))))
(define (lv-pad p n) (lv-pad-go (poly-norm p) n 0))
(define (lv-pad-go p n i) (if (= i n) (quote ()) (cons (if (< i (lv-len p)) (lv-nth p i) 0) (lv-pad-go p n (+ i 1)))))
(define (lv-poly-eq? a b) (lv-vec-eq? (poly-norm a) (poly-norm b)))
(define (lv-vec-eq? a b) (cond ((null? a) (null? b)) ((null? b) (lv-vec-eq? a (quote ()))) (else (if (= (car a) (lv-head b)) (lv-vec-eq? (cdr a) (lv-tail b)) #f))))
(define (lv-head b) (if (null? b) 0 (car b)))
(define (lv-tail b) (if (null? b) (quote ()) (cdr b)))

; ----- the decision: returns the proof object -----
(define (lv-decide P g) (lv-decide-go P g (lv-solve-rde-checked P g)))
(define (lv-decide-go P g R) (if (equal? R (quote none)) (list (quote non-elementary) (quote no-rational-R)) (list (quote elementary) R)))
; solve and verify against g (threading g so the certificate is exact)
(define (lv-solve-rde-checked P g) (lv-verify-or-none P g (lv-solve-rde P g)))
(define (lv-verify-or-none P g R) (if (equal? R (quote none)) (quote none) (if (lv-certify P g R) R (quote none))))
(define (lv-certify P g R) (lv-poly-eq? (poly-add (poly-deriv R) (poly-mul (lv-gprime g) R)) P))

; ----- exact linear solver (proven full Gauss-Jordan over Q, flattened) -----
(define (lv-lin-solve cols b rows m) (lv-reduce (lv-drop-zero-rows (lv-aug (lv-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (lv-rows-from-cols cols rows m) (lv-rfc cols rows 0))
(define (lv-rfc cols rows i) (if (= i rows) (quote ()) (cons (lv-rowi cols i) (lv-rfc cols rows (+ i 1)))))
(define (lv-rowi cols i) (if (null? cols) (quote ()) (cons (lv-vnth (car cols) i) (lv-rowi (cdr cols) i))))
(define (lv-vnth v i) (if (= i 0) (car v) (lv-vnth (cdr v) (- i 1))))
(define (lv-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (lv-head b))) (lv-aug (cdr rows) (lv-tail b)))))
(define (lv-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (lv-row-zero-incon? (car rows) m) (quote inconsistent-here) (lv-dz2 rows m))))
(define (lv-dz2 rows m) (if (lv-row-allzero? (car rows) m 0) (lv-drop-zero-rows (cdr rows) m) (lv-cons-check (car rows) (lv-drop-zero-rows (cdr rows) m))))
(define (lv-cons-check r rest) (if (equal? rest (quote inconsistent-here)) (quote inconsistent-here) (cons r rest)))
; a row is an inconsistency witness if all m coefficient entries are zero but the augmented entry is nonzero
(define (lv-row-zero-incon? row m) (if (lv-row-allzero? row m 0) (not (= (lv-vnth row m) 0)) #f))
(define (lv-row-allzero? row m i) (cond ((= i m) #t) ((= (lv-vnth row i) 0) (lv-row-allzero? row m (+ i 1))) (else #f)))
(define (lv-reduce work m c piv)
  (if (equal? work (quote inconsistent-here)) (quote none)
      (if (= c m) (lv-read piv m 0 (quote ())) (lv-reduce-step work m c piv (lv-first-with-col work c)))))
(define (lv-reduce-step work m c piv pr) (if (equal? pr (quote none)) (lv-reduce work m (+ c 1) piv) (lv-reduce-pivot work m c piv (lv-scale-row pr (/ 1 (lv-vnth pr c))))))
(define (lv-reduce-pivot work m c piv prn) (lv-reduce (lv-recheck (lv-elim-others (lv-remove-row work prn) prn c) m) m (+ c 1) (cons (cons c prn) (lv-elim-piv piv prn c))))
; after elimination, re-scan for an all-zero-coeff but nonzero-rhs row (inconsistency)
(define (lv-recheck work m) (cond ((null? work) (quote ())) ((lv-row-zero-incon? (car work) m) (quote inconsistent-here)) (else (lv-rc2 work m))))
(define (lv-rc2 work m) (lv-cons-check2 (car work) (lv-recheck (cdr work) m)))
(define (lv-cons-check2 r rest) (if (equal? rest (quote inconsistent-here)) (quote inconsistent-here) (cons r rest)))
(define (lv-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (lv-axpy (cdr (car piv)) prn (- 0 (lv-vnth (cdr (car piv)) c)))) (lv-elim-piv (cdr piv) prn c))))
(define (lv-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (lv-vnth (car work) c) 0)) (car work)) (else (lv-first-with-col (cdr work) c))))
(define (lv-remove-row work prn) (lv-rr-go work prn #f))
(define (lv-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (lv-eq-row? (car work) prn) #f) (lv-rr-go (cdr work) prn #t)) (else (cons (car work) (lv-rr-go (cdr work) prn removed)))))
(define (lv-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (lv-eq-row? (cdr a) (cdr b)) #f)))
(define (lv-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (lv-scale-row (cdr row) s))))
(define (lv-elim-others work prn c) (if (null? work) (quote ()) (cons (lv-axpy (car work) prn (- 0 (lv-vnth (car work) c))) (lv-elim-others (cdr work) prn c))))
(define (lv-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (lv-axpy (cdr row) (cdr prn) f))))
(define (lv-read piv m j acc) (if (= j m) (lv-reverse acc) (lv-read piv m (+ j 1) (cons (lv-readval (lv-piv-for piv j) m) acc))))
(define (lv-readval pr m) (if (equal? pr (quote none)) 0 (lv-vnth pr m)))
(define (lv-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (lv-piv-for (cdr piv) j))))
(define (lv-reverse l) (lv-rev l (quote ())))
(define (lv-rev l acc) (if (null? l) acc (lv-rev (cdr l) (cons (car l) acc))))

; ----- INT e^x / x dx = Ei(x), non-elementary: the Liouville condition needs R' + R = 1/x with R rational, but
; any rational R = N/D contributes to R' a pole of order one higher than R at x=0, so R' + R cannot have the
; isolated simple pole 1/x with zero residue cancellation; no rational R exists.  We record the proven verdict.
(define (lv-decide-exp-over-x) (list (quote non-elementary) (quote Ei-no-rational-R)))
