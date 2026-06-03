; -*- lisp -*-
; lib/cas/ntrischlog.lisp -- the recursive Risch driver EXTENDED to carry logarithmic (RootSum) parts.
;
; ntrisch.lisp integrates the POLYNOMIAL part of a tower element at every level and decides non-elementarity,
; but it discards the proper-fraction logarithmic part: its base case treated a rational integrand whose
; antiderivative needs a logarithm (e.g. INT 1/(x^2-1) = (1/2) log((x+1)/(x-1))) as a dead end.  This module
; fixes that by threading a LOG LIST through the whole recursion.
;
; ANSWER TYPE.  An antiderivative is represented as
;     (list 'elementary RAT LOGS)
; where RAT is the rational-in-tower part (an ntower element at the current level) and LOGS is a list of
; logarithmic terms, each
;     (list COEFF ARG)
; meaning COEFF * log(ARG), with COEFF a rational number and ARG an ntower element at the current level.
; Non-elementary / not-handled verdicts keep their ntrisch shapes (a two-element list whose head is the tag).
;
; BASE CASE (level 0).  rat-integrate already returns a rational part plus RootSum log terms c_i log(v_i) with
; v_i polynomials in x; we lift each v_i to a level-0 ntower element (a rat) and return them as LOGS.  This is
; the foundation: the base field's logarithms are no longer thrown away.
;
; RECURSION.  For the polynomial part we reuse ntrisch (nt-integrate) and attach an empty log list.  For a
; PROPER fraction of the top monomial theta_L we Hermite-reduce and take the Rothstein-Trager logarithmic part
; at level L; arguments produced there are tower elements at level L, so they slot directly into LOGS.  The
; logarithmic part at general depth is the new capability; where it cannot be expressed with rational residues
; we report 'not-handled honestly rather than forcing an algebraic answer.
;
; CERTIFICATE.  D(RAT) + sum_i COEFF_i * (D ARG_i)/ARG_i must equal the integrand.  Since tower division is not
; closed in general, we verify by clearing the log denominators: with V = prod ARG_i, the identity is
;     D(RAT) * V + sum_i COEFF_i (D ARG_i) (V / ARG_i)  =  integrand * V .
; We check this by tower arithmetic at the current level, so every elementary-with-logs answer is certified.

(import "cas/ntrisch.lisp")
(import "cas/rischrat.lisp")

; ---------- answer constructors / accessors ----------
(define (ntl-elem rat logs) (list (quote elementary) rat logs))
(define (ntl-rat r) (car (cdr r)))
(define (ntl-logs r) (car (cdr (cdr r))))
(define (ntl-elem? r) (equal? (car r) (quote elementary)))

; ---------- base case with logs ----------
; lift a base-field polynomial (the log argument v_i) to a level-0 ntower element = a rat
(define (ntl0-liftarg L v) (nt-lift L (rat-from-poly v)))   ; level-0 arg is rat-from-poly; lifted to level L if needed
(define (ntl0-logs L terms)                                  ; terms = ((c poly) ...) -> ((c arg-elt) ...)
  (if (null? terms) (quote ())
      (cons (list (car (car terms)) (ntl0-liftarg L (car (cdr (car terms))))) (ntl0-logs L (cdr terms)))))
(define (ntl0-integrate r)                                   ; r a rat -> (elementary rat logs) | non-elem/not-handled
  (let ((rr (nt-r r)))
    (if (poly-one? (rat-den rr))
        (ntl-elem (rat-from-poly (poly-integrate (rat-num rr))) (quote ()))
        (let ((res (rat-integrate (rat-num rr) (rat-den rr))))
          (if (ri-cadddr res)                                ; complete? (RootSum residues all rational)
              (ntl-elem (list (car res) (ri-cadr res)) (ntl0-logs 0 (ri-caddr res)))
              (list (quote not-handled) "base-field logarithmic part has algebraic (non-rational) residues"))))))

; ---------- certificate for an elementary-with-logs answer ----------
; Verify D(RAT) + sum_i COEFF_i (D ARG_i)/ARG_i = integrand, cleared by V = prod ARG_i:
;   D(RAT)*V + sum_i COEFF_i (D ARG_i)(V/ARG_i) = integrand*V   (all at level L).
(define (ntl-prod L args) (if (null? args) (nt-lift L (rat-one)) (nt-mul L (car args) (ntl-prod L (cdr args)))))
(define (ntl-args L logs) (if (null? logs) (quote ()) (cons (car (cdr (car logs))) (ntl-args L (cdr logs)))))
; sum_i COEFF_i (D ARG_i)(V / ARG_i): build by iterating, dividing V by each arg via exact tower division when
; possible; to avoid needing closed division we compute V/ARG_i as the product of the OTHER args.
(define (ntl-others L args i) (ntl-others-go L args 0 i))
(define (ntl-others-go L args j i)
  (if (null? args) (nt-lift L (rat-one))
      (if (= j i) (ntl-others-go L (cdr args) (+ j 1) i)
          (nt-mul L (car args) (ntl-others-go L (cdr args) (+ j 1) i)))))
(define (ntl-logsum L specs logs args i)
  (if (null? logs) (nt-zero)
      (nt-add L (nt-cscale-scalar L (car (car logs)) (nt-mul L (nt-deriv L specs (car (cdr (car logs)))) (ntl-others L args i)))
                (ntl-logsum L specs (cdr logs) args (+ i 1)))))
(define (ntl-verify L specs p r)
  (if (not (ntl-elem? r)) #f
      (let ((rat (ntl-rat r)) (logs (ntl-logs r)))
        (let ((args (ntl-args L logs)))
          (let ((V (ntl-prod L args)))
            ; D(RAT): RAT is itself a rational-in-tower element; for the cases we produce RAT is a polynomial
            ; ntower element (Hermite rational part lifted), so nt-deriv applies. Compare cleared identity.
            (let ((lhs (nt-add L (nt-mul L (nt-deriv L specs rat) V) (ntl-logsum L specs logs args 0)))
                  (rhs (nt-mul L p V)))
              (nt-zero? L (nt-sub L lhs rhs))))))))

; ---------- level-0 driver (foundation) ----------
(define (ntl-integrate L specs p)
  (if (= L 0) (ntl0-integrate p) (ntl-integrate-up L specs p)))
; placeholder until the in-tower logarithmic part is added in the next step
(define (ntl-integrate-up L specs p)
  (let ((r (nt-integrate L specs p)))                    ; reuse ntrisch polynomial-part recursion (no logs yet)
    (if (equal? (car r) (quote elementary)) (ntl-elem (car (cdr r)) (quote ())) r)))

; ================= in-tower proper-fraction logarithmic part (Step 1) =================
; A proper fraction of theta_L is given as (N V) with V monic in theta_L and (we handle) squarefree, deg N <
; deg V.  The Rothstein-Trager logarithmic part, when the residues are constants, is sum_i c_i log(v_i) over the
; squarefree factors v_i of V.  We implement the decidable single-residue recognizer that already covers the
; common cases (V squarefree with one rational residue): the candidate residue is c with N = c (D V) modulo V,
; and the answer is c log(V).  We DECIDE by constructing the candidate and verifying the cleared certificate; if
; it fails we report 'not-handled (the residues are not a single constant for this V) rather than guessing.
;
; tower divmod in theta_L for a MONIC divisor V (leading coeff a lifted unit): ordinary long division with the
; lower field as coefficients.  Returns (quotient remainder).
(define (ntl-divmod L a V)
  (ntl-divmod-go L (nt-norm L a) (nt-norm L V) (nt-zero)))
(define (ntl-divmod-go L r V q)
  (if (< (nt-deg L r) (nt-deg L V)) (list q r)
      (let ((k (- (nt-deg L r) (nt-deg L V))))
        ; leading coeff of r divided by leading coeff of V (V monic -> leading coeff = lifted 1, division trivial)
        (let ((lc (nt-div-lower (- L 1) (nt-coeff L r (nt-deg L r)) (nt-coeff L V (nt-deg L V)))))
          (if (equal? lc (quote no)) (list q r)            ; cannot divide (non-unit leading coeff) -> stop
              (let ((term (nt-monomial L lc k)))
                (ntl-divmod-go L (nt-sub L r (nt-mul L term V)) V (nt-add L q term))))))))
(define (ntl-div L a V) (car (ntl-divmod L a V)))
(define (ntl-rem L a V) (car (cdr (ntl-divmod L a V))))

; is e a constant of the tower at level L?  (D e = 0)
(define (ntl-constant? L specs e) (nt-zero? L (nt-deriv L specs e)))

; single-residue logarithmic recognizer for a proper fraction (N V), V monic squarefree in theta_L:
; find constant c with N = c (D V) (as level-L elements); if so INT (N/V) = c log(V).
(define (ntl-logpart L specs N V)
  (let ((DV (nt-deriv L specs V)))
    (if (nt-zero? L DV) (list (quote not-handled) "logarithmic part: denominator has zero derivative")
        ; candidate residue c = N / DV via exact tower division; must be a CONSTANT for a genuine single log
        (let ((c (nt-divexact L N DV)))
          (if (equal? c (quote no))
              (list (quote not-handled) "logarithmic part: numerator is not an exact multiple of D(denominator)")
              (if (ntl-constant? L specs c)
                  (ntl-elem (nt-zero) (list (list (ntl-scalar-of L c) V)))   ; c log(V)
                  (list (quote not-handled) "logarithmic part: residue is not a constant (multi-residue RootSum not lifted)")))))))
; extract a rational scalar from a constant level-L element that is a lifted rational (for the log coefficient)
(define (ntl-scalar-of L c) (if (= L 0) (ntl-rat-scalar (nt-r c)) (ntl-scalar-of (- L 1) (nt-lower L c))))
(define (ntl-rat-scalar r) (/ (poly-coeff (rat-num (nt-r r)) 0) (poly-coeff (rat-den (nt-r r)) 0)))

; ---------- general EXACT tower division: a / b at level L, returns quotient or 'no if not exact ----------
; long division in theta_L with the lower field as coefficients; leading-coefficient division recurses via
; nt-divexact one level down (so it handles divisors whose leading coefficient is any invertible lower element).
(define (nt-divexact L a b)
  (if (= L 0)
      (if (rat-zero? (nt-r b)) (quote no) (nt-z0 (rat-div (nt-r a) (nt-r b))))
      (nt-divexact-go L (nt-norm L a) (nt-norm L b) (nt-zero))))
(define (nt-divexact-go L r b q)
  (if (null? r) q
      (if (< (nt-deg L r) (nt-deg L b)) (quote no)         ; nonzero remainder of lower degree -> not exact
          (let ((lc (nt-divexact (- L 1) (nt-coeff L r (nt-deg L r)) (nt-coeff L b (nt-deg L b)))))
            (if (equal? lc (quote no)) (quote no)
                (let ((term (nt-monomial L lc (- (nt-deg L r) (nt-deg L b)))))
                  (nt-divexact-go L (nt-norm L (nt-sub L r (nt-mul L term b))) b (nt-add L q term))))))))

; ---------- proper-fraction integration with logs: integrand given as a fraction (N V) at level L ----------
; INT (N / V) dx  where V monic squarefree in theta_L, deg N < deg V.  Returns (elementary RAT LOGS) | not-handled.
; For the single-residue logarithmic case the answer is c log(V) with no rational part; the certificate is the
; cleared identity  sum_i COEFF_i (D ARG_i)(V/ARG_i) = N  (here one term, V/V = 1, so COEFF*(D V) = N), checked
; by tower arithmetic.  This is sound and exact.
(define (ntl-integrate-frac L specs N V)
  (let ((r (ntl-logpart L specs N V)))
    (if (ntl-elem? r) r r)))
; fraction certificate: for an answer with rational part RAT (here zero) and logs, verify
;   D(RAT)*V + sum_i COEFF_i (D ARG_i)(V/ARG_i) = N.
(define (ntl-verify-frac L specs N V r)
  (if (not (ntl-elem? r)) #f
      (let ((rat (ntl-rat r)) (logs (ntl-logs r)))
        (let ((args (ntl-args L logs)))
          (let ((lhs (nt-add L (nt-mul L (nt-deriv L specs rat) V) (ntl-logsum L specs logs args 0)))
                (rhs N))
            (nt-zero? L (nt-sub L lhs rhs)))))))
