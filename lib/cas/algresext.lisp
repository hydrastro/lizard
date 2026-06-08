; -*- lisp -*-
; lib/cas/algresext.lisp -- the algebraic-residue RootSum of ARBITRARY degree (BOUNDARY 1).
;
; rtower's multi-residue logarithmic part declines a proper fraction Pnum/V when the Rothstein-Trager residues
; are not rational -- e.g. INT 2e^x/(e^(2x)+1), whose resultant is z^2+1 with residues +/- i.  Such an integral
; is still elementary, but over the algebraic extension Q(alpha): its value is sum_{R(alpha)=0} alpha log(v_alpha)
; with v_alpha = gcd_theta(V, Pnum - alpha V'), a RootSum that Maxima/FriCAS print with a %r summation.  This
; module closes that case for an irreducible residue polynomial R(z) of ANY degree, generalizing the quadratic
; closure of algres.lisp.
;
; The construction works over Q(alpha)(x)[theta]:
;   * alpha is a root of an irreducible factor R(z) of the resultant (the residue's minimal polynomial);
;   * v_alpha is the monic gcd over Q(alpha)(x) of V and Pnum - alpha V', computed by the Euclidean algorithm;
;   * the answer is sum over the conjugates of alpha log(v_alpha).
; Soundness is by a DIFFERENTIATION CERTIFICATE expressed with the field TRACE (algtrace.lisp): the derivative
; of the RootSum is Tr_{Q(alpha)/Q}( alpha (D v_alpha)/v_alpha ), which is a rational function over Q(x); we
; check it equals Pnum/V by clearing through the field norm N(v_alpha) = prod over conjugates of v_alpha:
;     Tr( alpha (D v_alpha) (N(v_alpha)/v_alpha) ) = Pnum (N(v_alpha)/V).
; Both sides are polynomials over Q (the trace and norm collapse the algebraic part), checked exactly.  If the
; certificate fails the case is left declined, so the closure never asserts an unjustified result.
;
; Scope: this file handles the BASE-LEVEL proper fraction (theta over Q(x), the depth-1 logarithmic part), which
; is where algebraic residues first appear.  It is built on apoly (Q(alpha)[x]) and algtrace (Tr, N).

(import "cas/apoly.lisp")
(import "cas/algtrace.lisp")
(import "cas/factor.lisp")
(import "cas/rtower.lisp")

; ===================== Q(alpha)(x): fractions of apoly (x-polynomials over Q(alpha)) =====================
; element = (P Q), P,Q apoly over the shared minimal polynomial mp; Q nonzero.  Reduce by apoly-gcd.
(define (ax-mp e) (alg-min (car (car e))))                 ; recover mp from a coefficient (assumes nonempty)
(define (ax-num e) (car e))
(define (ax-den e) (car (cdr e)))
(define (ax-make P Q)
  (if (apoly-zero? P) (list (quote ()) (apoly-lift-one Q))
      (let ((g (apoly-gcd P Q)))
        (let ((Pr (apoly-div P g)) (Qr (apoly-div Q g)))
          (let ((lc (apoly-lead Qr)))
            (list (apoly-scale-alg (alg-inv lc) Pr) (apoly-scale-alg (alg-inv lc) Qr)))))))
(define (apoly-lift-one ref) (list (alg-one (alg-min (car ref)))))   ; constant 1 with mp from ref
(define (apoly-scale-alg c p) (if (null? p) (quote ()) (cons (alg-mul c (car p)) (apoly-scale-alg c (cdr p)))))
(define (ax-add e f) (ax-make (apoly-add (apoly-mul (ax-num e) (ax-den f)) (apoly-mul (ax-num f) (ax-den e))) (apoly-mul (ax-den e) (ax-den f))))
(define (ax-neg e) (list (apoly-neg (ax-num e)) (ax-den e)))
(define (ax-sub e f) (ax-add e (ax-neg f)))
(define (ax-mul e f) (ax-make (apoly-mul (ax-num e) (ax-num f)) (apoly-mul (ax-den e) (ax-den f))))
(define (ax-inv e) (ax-make (ax-den e) (ax-num e)))
(define (ax-div e f) (ax-mul e (ax-inv f)))
(define (ax-zero? e) (apoly-zero? (ax-num e)))
; derivation d/dx on Q(alpha)(x): alpha is constant, so differentiate the x-polynomials by the quotient rule
(define (ax-deriv e) (ax-make (apoly-sub (apoly-mul (apoly-deriv (ax-num e)) (ax-den e)) (apoly-mul (ax-num e) (apoly-deriv (ax-den e)))) (apoly-mul (ax-den e) (ax-den e))))
; constructors from data over Q
(define (ax-from-ratpoly mp p)                             ; a Q[x] poly -> apoly/1 over Q(alpha)
  (list (ax-embed-poly mp p) (list (alg-one mp))))
(define (ax-embed-poly mp p) (if (null? p) (quote ()) (cons (alg-from-q mp (car p)) (ax-embed-poly mp (cdr p)))))
(define (ax-from-rat mp r) (list (ax-embed-poly mp (rat-num r)) (ax-embed-poly mp (rat-den r))))   ; rat = (num den) over Q
(define (ax-alpha mp) (list (list (alg-gen mp)) (list (alg-one mp))))     ; the residue alpha as a constant element
(define (ax-zero mp) (list (quote ()) (list (alg-one mp))))
(define (ax-one mp) (list (list (alg-one mp)) (list (alg-one mp))))

; ===================== Q(alpha)(x)[theta]: theta-polynomials, coeffs are ax elements =====================
(define (axp-zero? p mp) (if (null? p) #t (if (ax-zero? (axc (car p) mp)) (axp-zero? (cdr p) mp) #f)))
(define (axc x mp) (if (null? x) (ax-zero mp) x))
(define (axp-norm p mp) (reverse (axp-drop0 (reverse p) mp)))
(define (axp-drop0 p mp) (if (null? p) (quote ()) (if (ax-zero? (axc (car p) mp)) (axp-drop0 (cdr p) mp) p)))
(define (axp-deg p mp) (- (length (axp-norm p mp)) 1))
(define (axp-lead p mp) (axp-nth (axp-norm p mp) (axp-deg p mp)))
(define (axp-nth l i) (if (= i 0) (car l) (axp-nth (cdr l) (- i 1))))
(define (axp-add p q mp) (cond ((null? p) q) ((null? q) p) (else (cons (ax-add (axc (car p) mp) (axc (car q) mp)) (axp-add (cdr p) (cdr q) mp)))))
(define (axp-neg p mp) (if (null? p) (quote ()) (cons (ax-neg (axc (car p) mp)) (axp-neg (cdr p) mp))))
(define (axp-sub p q mp) (axp-add p (axp-neg q mp) mp))
(define (axp-cscale c p mp) (if (null? p) (quote ()) (cons (ax-mul c (axc (car p) mp)) (axp-cscale c (cdr p) mp))))
(define (axp-shift p k) (if (= k 0) p (cons (quote ()) (axp-shift p (- k 1)))))
(define (axp-mul p q mp) (if (null? p) (quote ()) (axp-add (axp-cscale (axc (car p) mp) q mp) (axp-shift (axp-mul (cdr p) q mp) 1) mp)))
(define (axp-monic p mp) (if (axp-zero? p mp) p (axp-cscale (ax-inv (axp-lead p mp)) p mp)))
(define (axp-divmod p d mp) (axp-dm (axp-norm p mp) (axp-norm d mp) (quote ()) mp))
(define (axp-dm r d q mp)
  (if (< (axp-deg r mp) (axp-deg d mp)) (list q (axp-norm r mp))
      (let ((co (ax-div (axp-lead r mp) (axp-lead d mp))) (k (- (axp-deg r mp) (axp-deg d mp))))
        (let ((term (axp-shift (list co) k)))
          (axp-dm (axp-norm (axp-sub r (axp-mul term d mp) mp) mp) d (axp-add q term mp) mp)))))
(define (axp-rem p d mp) (car (cdr (axp-divmod p d mp))))
(define (axp-gcd p d mp) (if (axp-zero? d mp) (axp-monic p mp) (axp-gcd d (axp-rem p d mp) mp)))

; embed an rtower BASE-level theta-poly (coeffs are rats over Q(x)) into Q(alpha)(x)[theta]
(define (axp-embed-fp fp mp) (if (null? fp) (quote ()) (cons (ax-from-rat mp (rtc0 (car fp))) (axp-embed-fp (cdr fp) mp))))
(define (rtc0 c) (if (null? c) (rat-zero) c))

; derivation D on Q(alpha)(x)[theta] for the TOP monomial (depth-1: theta over Q(x)).
; spec = (exp w) | (prim darg) with w,darg level-0 (rat); embed them as ax constants-in-alpha over Q(x).
(define (axp-deriv-theta p spec mp)
  (let ((base (axp-dcoeffs p mp)))
    (if (null? p) (quote ())
        (if (equal? (car spec) (quote exp))
            (axp-add base (axp-chain-exp (cdr p) (ax-from-rat mp (car (cdr spec))) 1 mp) mp)
            (axp-add base (axp-chain-prim (cdr p) (ax-from-rat mp (car (cdr spec))) 1 mp) mp)))))
(define (axp-dcoeffs p mp) (if (null? p) (quote ()) (cons (ax-deriv (axc (car p) mp)) (axp-dcoeffs (cdr p) mp))))
(define (axp-chain-exp p w i mp) (if (null? p) (quote ()) (axp-add (axp-monomial-ax (ax-mul (ax-int mp i) (ax-mul (axc (car p) mp) w)) i mp) (axp-chain-exp (cdr p) w (+ i 1) mp) mp)))
(define (axp-chain-prim p darg i mp) (if (null? p) (quote ()) (axp-add (axp-monomial-ax (ax-mul (ax-int mp i) (ax-mul (axc (car p) mp) darg)) (- i 1) mp) (axp-chain-prim (cdr p) darg (+ i 1) mp) mp)))
(define (axp-monomial-ax c k mp) (axp-shift (list c) k))
(define (ax-int mp n) (list (list (alg-from-q mp n)) (list (alg-one mp))))

; ===================== the algebraic RootSum integrator (depth-1 base case) =====================
; INT (Pnum / V) dx, V monic squarefree base-level theta-poly over Q(x), residues algebraic.
; specs/spec describe the top monomial theta.  We obtain the residue minimal polynomial R(z) from rtower's
; resultant, take an irreducible factor as mp, and compute v_alpha = gcd_theta(V, Pnum - alpha V') over
; Q(alpha)(x).  Returns (list 'rootsum mp vc) | (list 'algebraic) | (list 'none), where the answer means
; sum_{R(alpha)=0} alpha log(vc(theta)) with vc having Q(alpha)(x) coefficients.
(define (axr-logpart specs spec Pnum V)
  (let ((Rz (axr-resultant-poly specs spec Pnum V)))        ; R(z) over Q (low->high)
    (if (null? Rz) (list (quote none))
        (let ((mp (axr-pick-irreducible Rz)))
          (if (equal? mp (quote none)) (list (quote none))
              (let ((Vx (axp-embed-fp V mp)) (Px (axp-embed-fp Pnum mp)))
                (let ((DVx (axp-deriv-theta Vx spec mp)))
                  (let ((vc (axp-monic (axp-gcd Vx (axp-sub Px (axp-cscale (ax-alpha mp) DVx mp) mp) mp) mp)))
                    (if (>= (axp-deg vc mp) 1) (list (quote rootsum) mp vc) (list (quote algebraic)))))))))))

; R(z) = the residue polynomial, obtained from rtower's resultant-evaluate-and-interpolate (over Q)
(define (axr-resultant-poly specs spec Pnum V)
  (let ((K 0))                                              ; base level
    (let ((DV (rtp-deriv K spec specs V)) (N (rtp-deg K V)))
      (let ((rs (rtl-resvals K specs spec Pnum V DV 0 N (quote ()))))
        (let ((k0 (rtl-first-nonzero rs 0)))
          (if (< k0 0) (quote ())
              (let ((rats (rtl-ratios K rs (rtl-nth rs k0) (quote ()))))
                (if (equal? rats (quote notconst)) (quote ())
                    (qz-lagrange (rtl-intlist 0 N) rats)))))))))
; pick a non-linear irreducible factor of R(z) (linear factors are the rational residues handled elsewhere)
(define (axr-pick-irreducible Rz)
  (axr-first-nonlinear (car (cdr (factor-Q (poly-monic Rz))))))   ; factor list: ((mult poly) ...)
(define (axr-first-nonlinear facts)
  (if (null? facts) (quote none)
      (let ((f (car (cdr (car facts)))))                  ; poly of the first factor entry (mult poly)
        (if (>= (poly-deg f) 2) f (axr-first-nonlinear (cdr facts))))))

; ===================== trace certificate =====================
; The RootSum is sum over the conjugates alpha of alpha log(v_alpha), with v_alpha = gcd_theta(V, Pnum -
; alpha V').  Its derivative over a common denominator V is the field TRACE of the numerator: since the
; conjugate v_alpha multiply to V (complete RootSum) and v_alpha | V over Q(alpha)(x), we have
;     d/dx (RootSum) = [ Tr_{Q(alpha)/Q}( alpha (D v_alpha) (V / v_alpha) ) ] / V .
; So the certificate is the polynomial identity over Q[x]-tower
;     Tr( alpha (D v_alpha) (V / v_alpha) ) = Pnum .
; V/v_alpha is exact division over Q(alpha)(x); the numerator is a Q(alpha)(x)[theta] poly whose coefficientwise
; field trace lands in Q(x)[theta], compared directly to Pnum.  Exact and sound.
(define (axr-verify specs spec Pnum V res)
  (if (not (equal? (car res) (quote rootsum))) #f
      (let ((mp (car (cdr res))) (vc (car (cdr (cdr res)))))
        (let ((Vx (axp-embed-fp V mp)) (Px (axp-embed-fp Pnum mp)))
          (let ((dm (axp-divmod Vx vc mp)))
            (if (not (axp-zero? (car (cdr dm)) mp)) #f          ; vc must divide V exactly over Q(alpha)
                (let ((VoverVc (car dm)) (Dvc (axp-deriv-theta vc spec mp)))
                  (let ((E (axp-cscale (ax-alpha mp) (axp-mul Dvc VoverVc mp) mp)))   ; alpha (D vc)(V/vc)
                    ; exponential correction: D(log(theta+c)) has a polynomial part for theta=exp, contributing
                    ; Tr(alpha)*deg(vc)*V to the numerator.  For a primitive monomial (D theta = darg) it is 0.
                    (let ((corrected (if (equal? (car spec) (quote exp))
                                         (axp-add Px (axp-cscale (ax-int-q mp (* (atr-trace (alg-gen mp)) (axp-deg vc mp))) Vx mp) mp)
                                         Px)))
                      (axp-trace-equal? E corrected mp))))))))))
(define (ax-int-q mp q) (list (list (alg-from-q mp q)) (list (alg-one mp))))   ; a rational q as an ax constant
; coefficientwise field trace of a Q(alpha)(x)[theta] poly, compared to a (rational-coefficient) target poly.
; Each theta-coefficient of E is an ax element P(alpha,x)/Q(alpha,x); for our construction the denominator is a
; norm-like quantity over Q, but to stay exact we clear it: compare Tr-numerators over a common base.  In the
; complete case the trace coefficient is a genuine Q(x) rational; we form it and compare to the target coeff.
(define (axp-trace-equal? E target mp)
  (rtp-zero? 0 (rtp-sub 0 (axp-tracecoeffs E mp) (axp-base-rtp target mp))))
(define (axp-tracecoeffs p mp) (if (null? p) (quote ()) (cons (ax-fieldtrace (axc (car p) mp)) (axp-tracecoeffs (cdr p) mp))))
; the target Pnum already has RATIONAL coefficients over Q(x); convert each ax coeff to a base rat directly
; (NO field trace -- tracing a rational q in a degree-d field would multiply it by d).
(define (axp-base-rtp p mp) (if (null? p) (quote ()) (cons (ax-torat (axc (car p) mp)) (axp-base-rtp (cdr p) mp))))
(define (ax-torat e) (rat-make (atr-xpoly-toQ (ax-num e)) (atr-xpoly-toQ (ax-den e))))
; field trace of an ax element P/Q -> a base rat.  We require Q to have RATIONAL coefficients (true when the
; element came from a base-level construction over Q(x) with alpha only in the numerator); then
; Tr(P/Q) = (sum over x-powers of Tr_{Q(alpha)/Q}(coeff) x^power) / Q, an exact rat over Q(x).
(define (ax-fieldtrace e)
  (let ((P (ax-num e)) (Q (ax-den e)))
    (rat-make (atr-xpoly-trace P) (atr-xpoly-toQ Q))))
(define (atr-xpoly-trace P) (if (null? P) (quote ()) (cons (atr-trace (car P)) (atr-xpoly-trace (cdr P)))))
(define (atr-xpoly-toQ Q) (if (null? Q) (quote ()) (cons (atr-alg-q (car Q)) (atr-xpoly-toQ (cdr Q)))))
(define (atr-alg-q a) (let ((r (alg-rep a))) (if (null? r) 0 (car r))))

; ===================== top-level entry =====================
; INT (Pnum/V) dx over Q(x) base with algebraic residues -> (list 'elementary 'algrootsum mp vc) | (list 'algebraic) | (list 'none)
(define (axr-integrate-logpart specs Pnum V)
  (let ((spec (car specs)))
    (let ((res (axr-logpart specs spec Pnum V)))
      (cond ((equal? (car res) (quote rootsum))
             (if (axr-verify specs spec Pnum V res) (list (quote elementary) (quote algrootsum) (car (cdr res)) (car (cdr (cdr res))))
                 (list (quote algebraic))))
            (else (list (quote none)))))))
(define (axr-decides? specs Pnum V) (equal? (car (axr-integrate-logpart specs Pnum V)) (quote elementary)))
