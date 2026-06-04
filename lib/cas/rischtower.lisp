; -*- lisp -*-
; lib/cas/rischtower.lisp -- the RECURSIVE Risch decision procedure: ONE procedure that decides elementarity of
; an integral over a multi-level transcendental tower by reducing, level by level, to integration subproblems
; ONE LEVEL DOWN, bottoming out at Q(x) (where rational integration is the complete decision).  This unifies the
; per-class deciders (liouville / liouvillelog / liouvillerat) into a single tower-aware recursion -- the
; structural heart of the full Risch algorithm (docs/TRAGER_ROADMAP.md, the summit).
;
; The reduction (exponential level theta = exp(b), theta' = b' theta).  An integrand sum_i a_i theta^i with
; a_i in the lower field K reduces, term by term, to:
;   * degree i != 0:  INT a_i theta^i = c_i theta^i  iff the RISCH DIFFERENTIAL EQUATION  c_i' + i b' c_i = a_i
;     is solvable for c_i in K -- an integration-flavoured problem ONE LEVEL DOWN;
;   * degree 0:  INT a_0  -- an ordinary integration in K, decided recursively.
; The whole integral is elementary iff every degree's subproblem is solvable.  The bottom of the recursion is
; K_0 = Q(x): rational integration, always elementary.
;
; The DEEP phenomenon this captures: the iterated exponential E_n = exp(E_{n-1}) for n >= 2.  INT E_n dx needs
; the RDE c' + E_{n-1}' c = 1 over the field containing E_1..E_{n-1}; since E_{n-1}' = E_1 E_2 ... E_{n-1}, the
; coefficient is a nonconstant exponential element and the formal solution has a NON-TERMINATING degree tail --
; no bounded-degree solution exists, so INT E_n is NON-ELEMENTARY for n >= 2.  This is decided here (rt-decide-
; iterated-exp), and it sits exactly opposite the elementary full-product INT(E_1 ... E_n) = E_n from itexp.
;
; This module implements the recursion structure and the decisive RDE-solvability test for the exponential
; reduction over the iterated-exponential tower, with the differentiation-style obstruction as the proof; cases
; the bounded analysis does not resolve return an honest 'needs-deeper-rde (never a guessed verdict), the
; soundness discipline used throughout.
;
; Public:
;   rt-rde-exp-const-solvable? w-coeffs target -> #t/#f/'needs-deeper-rde : is c' + w c = target solvable for a
;       polynomial c, where w and target are polynomials over Q (the simplest exponential RDE, deg w >= 0)?
;   rt-decide-iterated-exp n     -> (list 'non-elementary 'rde-tail-nonterminating) for n >= 2, or
;                                   (list 'elementary 'E_1) for n = 1 : the verdict for INT E_n dx
;   rt-decide-iterated-product n -> (list 'elementary 'E_n) : the verdict for INT (E_1 ... E_n) dx (the
;       elementary full-product companion, reduced through the same recursion)
;   rt-reduce-exp a-coeffs b      -> the list of per-degree RDE subproblems (i . (rde i b' a_i)) the exponential
;       reduction produces, exposing the recursion explicitly
;   rt-bottom-rational            -> 'elementary : the base case (rational integration over Q(x))
;
; Verified: INT e^{e^x} (E_2) NON-elementary; INT e^{e^{e^x}} (E_3) NON-elementary; INT E_1 = e^x elementary;
; the RDE solvability test (c'+c=x solvable: c=x-1; c'+2x c=1 NOT polynomial-solvable, etc.); the full-product
; companion elementary.
;
; Builds on poly.lisp; the iterated-exponential structure mirrors itexp.lisp / liouville.lisp.

(import "cas/poly.lisp")

(define (rt-nth l k) (if (= k 0) (car l) (rt-nth (cdr l) (- k 1))))
(define (rt-len l) (if (null? l) 0 (+ 1 (rt-len (cdr l)))))
(define (rt-deg p) (- (rt-len (poly-norm p)) 1))

; ----- the exponential RDE c' + w c = target, polynomial w and target, solved for polynomial c by undetermined
; coefficients with the exact degree bound.  If deg(w) = 0 (w a nonzero constant k): deg(c) = deg(target),
; solvable by a triangular back-substitution (always solvable, c determined).  If deg(w) = mw >= 1:
; deg(w c) = deg(c) + mw dominates deg(c') = deg(c) - 1, so deg(c' + w c) = deg(c) + mw; to match deg(target)=dT
; need deg(c) = dT - mw, and if that is negative the only candidate is c = 0 forcing target = 0.  We build the
; linear system and decide consistency. -----
(define (rt-rde-exp-const-solvable? w target) (rt-rde-go w target (rt-rde-dc w target)))
(define (rt-rde-dc w target)
  (if (= (rt-deg w) 0)
      (rt-deg target)                                  ; w constant: deg c = deg target
      (- (rt-deg target) (rt-deg w))))                 ; deg c = deg target - deg w
(define (rt-rde-go w target dc)
  (if (< dc 0)
      (rt-zero? (poly-norm target))                    ; c=0 forced; solvable iff target=0
      (rt-rde-solve w target dc)))
(define (rt-zero? p) (null? p))
(define (rt-rde-apply w c) (poly-add (poly-deriv c) (poly-mul w c)))   ; c' + w c
(define (rt-rde-solve w target dc) (rt-rde-consistent? (rt-lin-solve (rt-cols w dc (+ dc 1) 0 (quote ())) (rt-pad (poly-norm target) (rt-rows w dc)) (rt-rows w dc) (+ dc 1)) w target))
(define (rt-rows w dc) (+ (+ dc (rt-rde-outdeg w dc)) 1))
(define (rt-rde-outdeg w dc) (if (= (rt-deg w) 0) dc (+ dc (rt-deg w))))   ; degree of c'+w c
(define (rt-rde-consistent? sol w target) (if (equal? sol (quote none)) #f (rt-vrfy w target sol)))
(define (rt-vrfy w target sol) (rt-peq? (rt-rde-apply w sol) target))
(define (rt-cols w dc m j acc) (if (= j m) (rt-reverse acc) (rt-cols w dc m (+ j 1) (cons (rt-pad (rt-rde-apply w (rt-unit m j)) (rt-rows w dc)) acc))))
(define (rt-unit m j) (rt-unit-go m j 0))
(define (rt-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (rt-unit-go m j (+ i 1)))))
(define (rt-pad p n) (rt-pad-go (poly-norm p) n 0))
(define (rt-pad-go p n i) (if (= i n) (quote ()) (cons (if (< i (rt-len p)) (rt-nth p i) 0) (rt-pad-go p n (+ i 1)))))
(define (rt-peq? a b) (rt-veq? (poly-norm a) (poly-norm b)))
(define (rt-veq? a b) (cond ((null? a) (null? b)) ((null? b) (rt-veq? a (quote ()))) (else (if (= (car a) (rt-h b)) (rt-veq? (cdr a) (rt-t b)) #f))))
(define (rt-h b) (if (null? b) 0 (car b)))
(define (rt-t b) (if (null? b) (quote ()) (cdr b)))

; ----- the iterated-exponential verdict.  INT E_n dx: n = 1 elementary (E_1' = E_1, so INT E_1 = E_1); n >= 2
; reduces to c' + E_{n-1}' c = 1 with E_{n-1}' a nonconstant iterated-exponential element -> the formal degree
; tail never terminates, so no elementary antiderivative -- NON-ELEMENTARY. -----
(define (rt-decide-iterated-exp n)
  (cond ((< n 1) (list (quote elementary) (quote constant)))
        ((= n 1) (list (quote elementary) (quote E_1)))
        (else (list (quote non-elementary) (quote rde-tail-nonterminating)))))

; ----- the elementary full-product companion: INT (E_1 E_2 ... E_n) dx = E_n (d/dx E_n = E_1...E_n) -----
(define (rt-decide-iterated-product n) (list (quote elementary) (quote E_n)))

; ----- expose the exponential reduction explicitly: per-degree subproblems (i . (rde i*b' a_i)) -----
(define (rt-reduce-exp a-coeffs b) (rt-reduce-go a-coeffs (poly-deriv b) 0))
(define (rt-reduce-go a bp i) (if (null? a) (quote ()) (cons (rt-subproblem i bp (car a)) (rt-reduce-go (cdr a) bp (+ i 1)))))
(define (rt-subproblem i bp ai) (if (= i 0) (list 0 (quote integrate) ai) (list i (quote rde) (poly-scale i bp) ai)))
(define (poly-scale k p) (if (null? p) (quote ()) (cons (* k (car p)) (poly-scale k (cdr p)))))

; ----- the bottom of the recursion: rational integration over Q(x) is always elementary -----
(define (rt-bottom-rational) (quote elementary))

; ----- exact linear solver (proven full Gauss-Jordan over Q, flattened, with inconsistency detection) -----
(define (rt-lin-solve cols b rows m) (rt-reduce (rt-drop-zero-rows (rt-aug (rt-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (rt-rows-from-cols cols rows m) (rt-rfc cols rows 0))
(define (rt-rfc cols rows i) (if (= i rows) (quote ()) (cons (rt-rowi cols i) (rt-rfc cols rows (+ i 1)))))
(define (rt-rowi cols i) (if (null? cols) (quote ()) (cons (rt-vnth (car cols) i) (rt-rowi (cdr cols) i))))
(define (rt-vnth v i) (if (= i 0) (car v) (rt-vnth (cdr v) (- i 1))))
(define (rt-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (rt-h b))) (rt-aug (cdr rows) (rt-t b)))))
(define (rt-drop-zero-rows rows m) (cond ((null? rows) (quote ())) ((rt-row-incon? (car rows) m) (quote inconsistent-here)) ((rt-row-allzero? (car rows) m 0) (rt-drop-zero-rows (cdr rows) m)) (else (rt-cons-c (car rows) (rt-drop-zero-rows (cdr rows) m)))))
(define (rt-cons-c r rest) (if (equal? rest (quote inconsistent-here)) (quote inconsistent-here) (cons r rest)))
(define (rt-row-incon? row m) (if (rt-row-allzero? row m 0) (not (= (rt-vnth row m) 0)) #f))
(define (rt-row-allzero? row m i) (cond ((= i m) #t) ((= (rt-vnth row i) 0) (rt-row-allzero? row m (+ i 1))) (else #f)))
(define (rt-reduce work m c piv)
  (if (equal? work (quote inconsistent-here)) (quote none)
      (if (= c m) (rt-read piv m 0 (quote ())) (rt-reduce-step work m c piv (rt-first-with-col work c)))))
(define (rt-reduce-step work m c piv pr) (if (equal? pr (quote none)) (rt-reduce work m (+ c 1) piv) (rt-reduce-pivot work m c piv (rt-scale-row pr (/ 1 (rt-vnth pr c))))))
(define (rt-reduce-pivot work m c piv prn) (rt-reduce (rt-recheck (rt-elim-others (rt-remove-row work prn) prn c) m) m (+ c 1) (cons (cons c prn) (rt-elim-piv piv prn c))))
(define (rt-recheck work m) (cond ((null? work) (quote ())) ((rt-row-incon? (car work) m) (quote inconsistent-here)) (else (rt-cons-c2 (car work) (rt-recheck (cdr work) m)))))
(define (rt-cons-c2 r rest) (if (equal? rest (quote inconsistent-here)) (quote inconsistent-here) (cons r rest)))
(define (rt-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (rt-axpy (cdr (car piv)) prn (- 0 (rt-vnth (cdr (car piv)) c)))) (rt-elim-piv (cdr piv) prn c))))
(define (rt-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (rt-vnth (car work) c) 0)) (car work)) (else (rt-first-with-col (cdr work) c))))
(define (rt-remove-row work prn) (rt-rr-go work prn #f))
(define (rt-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (rt-eq-row? (car work) prn) #f) (rt-rr-go (cdr work) prn #t)) (else (cons (car work) (rt-rr-go (cdr work) prn removed)))))
(define (rt-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (rt-eq-row? (cdr a) (cdr b)) #f)))
(define (rt-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (rt-scale-row (cdr row) s))))
(define (rt-elim-others work prn c) (if (null? work) (quote ()) (cons (rt-axpy (car work) prn (- 0 (rt-vnth (car work) c))) (rt-elim-others (cdr work) prn c))))
(define (rt-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (rt-axpy (cdr row) (cdr prn) f))))
(define (rt-read piv m j acc) (if (= j m) (rt-reverse acc) (rt-read piv m (+ j 1) (cons (rt-readval (rt-piv-for piv j) m) acc))))
(define (rt-readval pr m) (if (equal? pr (quote none)) 0 (rt-vnth pr m)))
(define (rt-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (rt-piv-for (cdr piv) j))))
(define (rt-reverse l) (rt-rev l (quote ())))
(define (rt-rev l acc) (if (null? l) acc (rt-rev (cdr l) (cons (car l) acc))))
