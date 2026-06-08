; -*- lisp -*-
; lib/cas/rtower.lisp -- a RATIONAL differential tower of arbitrary depth: at every level the coefficient ring
; is a genuine FIELD, so polynomial GCD, resultants, and the Rothstein-Trager logarithmic part lift uniformly.
;
; The uniform tower of ntower.lisp keeps elements POLYNOMIAL in each monomial with polynomial lower
; coefficients, so the lower coefficient ring is not closed under division -- which blocks both the
; multi-residue RootSum (needs gcd over the lower field) and nested logarithms used as variables.  This module
; fixes the representation: a level-L element is a FRACTION of polynomials in theta_L whose coefficients are
; level-(L-1) rational-tower elements, bottoming out at the base field Q(x) (rat), which is already a field.
;
; LAYER 1 (this file, first half): polynomials in theta_L over the level-(L-1) FIELD.
;   A "fp" (field polynomial) at level L is a list low-to-high of level-(L-1) elements (the coefficients).
;   The zero fp is ().  Because the coefficients form a field we have monic normalization, exact division with
;   remainder, gcd, and resultants -- the classical field-Euclidean toolkit.
; LAYER 2 (second half): fractions (P Q) of fps, giving the field at level L; plus the recursive derivation.
;
; Level-(L-1) field operations are provided by this module recursively: at level 0 they are the rat-* ops on
; Q(x); above, they are the LAYER-2 fraction operations one level down.  We thread the level L explicitly.

(import "cas/tower.lisp")
(import "cas/rothstein.lisp")   ; for ros-rational-roots (rational roots of a Q[z] polynomial)

; ===================== the level-(L-1) FIELD interface =====================
; A level-L value is: level 0 -> a rat (Q(x)); level >=1 -> a fraction (P Q) of fps at level L (LAYER 2).
; These dispatch on L and delegate to rat-* at the base or to the fraction ops above the base.
; A value may arrive as the universal empty zero () at any level; coerce it to that level's zero on use.
(define (rt-r a) (if (null? a) (rat-zero) a))
(define (rt-c L a) (if (null? a) (rt-zero L) a))
(define (rt-zero L) (if (= L 0) (rat-zero) (list (quote ()) (rtp-one (- L 1)))))     ; 0/1
(define (rt-one L) (if (= L 0) (rat-one) (list (rtp-lift-one (- L 1)) (rtp-one (- L 1)))))
(define (rt-zero? L a) (if (null? a) #t (if (= L 0) (rat-zero? a) (rtp-zero? (- L 1) (car a)))))
(define (rt-add L a b) (if (= L 0) (rat-add (rt-r a) (rt-r b)) (rtf-add (- L 1) (rt-c L a) (rt-c L b))))
(define (rt-sub L a b) (if (= L 0) (rat-sub (rt-r a) (rt-r b)) (rtf-sub (- L 1) (rt-c L a) (rt-c L b))))
(define (rt-mul L a b) (if (= L 0) (rat-mul (rt-r a) (rt-r b)) (rtf-mul (- L 1) (rt-c L a) (rt-c L b))))
(define (rt-neg L a) (if (= L 0) (rat-neg (rt-r a)) (rtf-neg (- L 1) (rt-c L a))))
(define (rt-inv L a) (if (= L 0) (rat-inv (rt-r a)) (rtf-inv (- L 1) (rt-c L a))))
(define (rt-div L a b) (if (= L 0) (rat-div (rt-r a) (rt-r b)) (rtf-div (- L 1) (rt-c L a) (rt-c L b))))
(define (rt-equal? L a b) (rt-zero? L (rt-sub L a b)))

; ===================== LAYER 1: field polynomials (fp) in theta over a level-K field =====================
; K is the level of the COEFFICIENTS (so an fp here represents a polynomial in theta_{K+1}).
(define (rtp-zero) (quote ()))
(define (rtp-zero? K p) (if (null? p) #t (if (rt-zero? K (car p)) (rtp-zero? K (cdr p)) #f)))
(define (rtp-norm K p) (reverse (rtp-drop0 K (reverse p))))
(define (rtp-drop0 K p) (if (null? p) (quote ()) (if (rt-zero? K (car p)) (rtp-drop0 K (cdr p)) p)))
(define (rtp-deg K p) (- (length (rtp-norm K p)) 1))
(define (rtp-lead K p) (rtp-nth (rtp-norm K p) (rtp-deg K p)))
(define (rtp-nth lst i) (if (= i 0) (car lst) (rtp-nth (cdr lst) (- i 1))))
(define (rtp-one K) (list (rt-one K)))
(define (rtp-lift-one K) (list (rt-one K)))                     ; the constant polynomial 1
(define (rtp-lift K c) (list c))                                ; constant poly with coefficient c (level-K elt)
(define (rtp-monomial K c k) (rtp-shift (list c) k))            ; c theta^k
(define (rtp-shift p k) (if (= k 0) p (cons (rt-zero-marker) (rtp-shift p (- k 1)))))
(define (rt-zero-marker) (quote ()))                            ; () as a coefficient = zero at any level (coerced on use)

(define (rtp-add K p q)
  (cond ((null? p) q) ((null? q) p)
        (else (cons (rt-add K (rtc (car p)) (rtc (car q))) (rtp-add K (cdr p) (cdr q))))))
(define (rtc x) (if (null? x) (quote ()) x))                    ; coefficient accessor (zero stays ())
(define (rtp-neg K p) (if (null? p) (quote ()) (cons (rt-neg K (rtc (car p))) (rtp-neg K (cdr p)))))
(define (rtp-sub K p q) (rtp-add K p (rtp-neg K q)))
(define (rtp-cscale K c p) (if (null? p) (quote ()) (cons (rt-mul K c (rtc (car p))) (rtp-cscale K c (cdr p)))))
(define (rtp-mul K p q)
  (if (null? p) (quote ())
      (rtp-add K (rtp-cscale K (rtc (car p)) q) (rtp-shift (rtp-mul K (cdr p) q) 1))))

; monic normalization (divide by leading coefficient, a field element) and division with remainder
(define (rtp-monic K p) (if (rtp-zero? K p) p (rtp-cscale K (rt-inv K (rtp-lead K p)) p)))
(define (rtp-divmod K a b)
  (rtp-divmod-go K (rtp-norm K a) (rtp-norm K b) (rtp-zero)))
(define (rtp-divmod-go K r d q)
  (if (< (rtp-deg K r) (rtp-deg K d)) (list q (rtp-norm K r))
      (let ((c (rt-div K (rtp-lead K r) (rtp-lead K d))) (k (- (rtp-deg K r) (rtp-deg K d))))
        (let ((term (rtp-monomial K c k)))
          (rtp-divmod-go K (rtp-norm K (rtp-sub K r (rtp-mul K term d))) d (rtp-add K q term))))))
(define (rtp-div K a b) (car (rtp-divmod K a b)))
(define (rtp-rem K a b) (car (cdr (rtp-divmod K a b))))
(define (rtp-gcd K a b) (if (rtp-zero? K b) (rtp-monic K a) (rtp-gcd K b (rtp-rem K a b))))

; ===================== LAYER 2: fractions (P Q) of fps over level K = L-1 (an element at level L=K+1) =====================
; Represent a level-(K+1) element as (list P Q) with P,Q fps over level K, Q nonzero.  Reduce by the fp-gcd.
(define (rtf-num e) (car e))
(define (rtf-den e) (car (cdr e)))
(define (rtf-make K P Q)                                       ; reduce P/Q to lowest terms, normalize den monic
  (if (rtp-zero? K P) (list (quote ()) (rtp-one K))
      (let ((g (rtp-gcd K P Q)))
        (let ((Pr (rtp-div K P g)) (Qr (rtp-div K Q g)))
          (let ((lc (rtp-lead K Qr)))                          ; make denominator monic (push unit into numerator)
            (list (rtp-cscale K (rt-inv K lc) Pr) (rtp-cscale K (rt-inv K lc) Qr)))))))
(define (rtf-add K a b)
  (rtf-make K (rtp-add K (rtp-mul K (rtf-num a) (rtf-den b)) (rtp-mul K (rtf-num b) (rtf-den a)))
              (rtp-mul K (rtf-den a) (rtf-den b))))
(define (rtf-neg K a) (list (rtp-neg K (rtf-num a)) (rtf-den a)))
(define (rtf-sub K a b) (rtf-add K a (rtf-neg K b)))
(define (rtf-mul K a b) (rtf-make K (rtp-mul K (rtf-num a) (rtf-num b)) (rtp-mul K (rtf-den a) (rtf-den b))))
(define (rtf-inv K a) (rtf-make K (rtf-den a) (rtf-num a)))
(define (rtf-div K a b) (rtf-mul K a (rtf-inv K b)))

; ===================== the recursive derivation =====================
; specs: innermost first, same shape as ntower -- (prim darg) | (exp w), but darg/w are RATIONAL-TOWER elements
; one level below the monomial they describe.  D theta_{K+1}: (prim) darg ; (exp) w * theta_{K+1}.
; D of an fp (polynomial in theta_{K+1}): coefficientwise D (recurses to level K) + chain rule.
(define (rtp-dcoeffs K specs p) (if (null? p) (quote ()) (cons (rt-deriv K specs (rtc (car p))) (rtp-dcoeffs K specs (cdr p)))))
; primitive chain: sum_{i>=1} i a_i darg theta^{i-1}
(define (rtp-chain-prim K darg p i)
  (if (null? p) (quote ())
      (rtp-add K (rtp-monomial K (rt-mul K (rt-iscale K i (rtc (car p))) darg) (- i 1))
                 (rtp-chain-prim K darg (cdr p) (+ i 1)))))
; exponential chain: sum_{i>=1} i a_i w theta^i
(define (rtp-chain-exp K w p i)
  (if (null? p) (quote ())
      (rtp-add K (rtp-monomial K (rt-mul K (rt-iscale K i (rtc (car p))) w) i)
                 (rtp-chain-exp K w (cdr p) (+ i 1)))))
(define (rt-iscale K n a) (rt-mul K (rt-from-int K n) a))
(define (rt-from-int K n) (if (= K 0) (list (poly-scale n (list 1)) (list 1)) (list (rtp-lift (- K 1) (rt-from-int (- K 1) n)) (rtp-one (- K 1)))))
; D of an fp at coefficient-level K (so the polynomial is in theta_{K+1}), given the spec for theta_{K+1}
(define (rtp-deriv K spec specs p)
  (let ((base (rtp-dcoeffs K specs p)))
    (if (null? p) (quote ())
        (if (equal? (car spec) (quote prim))
            (rtp-add K base (rtp-chain-prim K (car (cdr spec)) (cdr p) 1))
            (rtp-add K base (rtp-chain-exp K (car (cdr spec)) (cdr p) 1))))))
; D of a level-L element (a fraction at level L, i.e. K = L-1): quotient rule
(define (rt-deriv L specs e)
  (if (= L 0) (rat-deriv e)
      (let ((K (- L 1)) (spec (rt-nth specs (- L 1))))
        (let ((P (rtf-num e)) (Q (rtf-den e)))
          (let ((dP (rtp-deriv K spec specs P)) (dQ (rtp-deriv K spec specs Q)))
            ; (P/Q)' = (P' Q - P Q')/Q^2
            (rtf-make K (rtp-sub K (rtp-mul K dP Q) (rtp-mul K P dQ)) (rtp-mul K Q Q)))))))
(define (rt-nth lst i) (if (= i 0) (car lst) (rt-nth (cdr lst) (- i 1))))

; ===================== lifting / embedding =====================
; lift a level-K element c to a level-(K+1) element: the constant fraction c/1
(define (rt-lift1 K c) (list (rtp-lift K c) (rtp-one K)))
; the monomial theta_L itself as a level-L element: (theta)/1 = fp [0, 1] over level L-1
(define (rt-theta L) (list (list (rt-zero (- L 1)) (rt-one (- L 1))) (rtp-one (- L 1))))


; ----- rationals-only Lagrange interpolation: xs integers, ys Scheme rationals -> Q[z] poly low->high -----
(define (qz-nth lst i) (if (= i 0) (car lst) (qz-nth (cdr lst) (- i 1))))
(define (qz-basis-num xs k j acc) (if (null? xs) acc
   (if (= j k) (qz-basis-num (cdr xs) k (+ j 1) acc) (qz-basis-num (cdr xs) k (+ j 1) (poly-mul acc (list (- 0 (car xs)) 1))))))
(define (qz-basis-den xs k j xk acc) (if (null? xs) acc
   (if (= j k) (qz-basis-den (cdr xs) k (+ j 1) xk acc) (qz-basis-den (cdr xs) k (+ j 1) xk (* acc (- xk (car xs)))))))
(define (qz-lag-go xs ys k acc) (if (>= k (length xs)) acc
   (qz-lag-go xs ys (+ k 1) (poly-add acc (poly-scale (/ (qz-nth ys k) (qz-basis-den xs k 0 (qz-nth xs k) 1)) (qz-basis-num xs k 0 (list 1)))))))
(define (qz-lagrange xs ys) (qz-lag-go xs ys 0 (quote ())))

; ===================== resultant and multi-residue logarithmic part (Target 1) =====================
; Resultant of two fps over the level-K field, via the Euclidean (subresultant-free) approach: since the
; coefficients form a field, res(f,g) can be computed from the remainder sequence.  We only need res to VANISH
; or not and, more importantly, the value res(V, N - c V') as a function of c; we obtain that function by
; evaluating at integer c and interpolating (the residues are constants, so this is exact and finite).
; The single resultant value res(f,g) over a field: product form via the remainder sequence.
(define (rtp-resultant K f g)
  (let ((f1 (rtp-norm K f)) (g1 (rtp-norm K g)))
    (if (if (rtp-zero? K f1) #t (rtp-zero? K g1)) (rt-zero K)
        (if (= (rtp-deg K g1) 0) (rt-pow K (rtp-lead K g1) (rtp-deg K f1))
            (rtp-res-go K f1 g1 (rt-one K))))))
(define (rt-pow K a n) (if (= n 0) (rt-one K) (rt-mul K a (rt-pow K a (- n 1)))))
; remainder-sequence resultant: res(f,g) = (-1)^{deg f deg g} lead(g)^{deg f - deg r} res(g, r), r = f mod g
(define (rtp-res-go K f g acc)
  (let ((r (rtp-rem K f g)))
    (let ((df (rtp-deg K f)) (dg (rtp-deg K g)))
      (if (rtp-zero? K r) (rt-zero K)
          (let ((sign (if (= (remainder (* df dg) 2) 0) (rt-one K) (rt-neg K (rt-one K)))))
            (let ((fac (rt-mul K sign (rt-pow K (rtp-lead K g) (- df (rtp-deg K r))))))
              (if (= (rtp-deg K r) 0)
                  (rt-mul K acc (rt-mul K fac (rt-pow K (rtp-lead K r) dg)))
                  (rt-mul K acc (rt-mul K fac (rtp-res-go K g r (rt-one K)))))))))))

; the logarithmic part of a proper fraction Pnum/V (V monic squarefree in theta, deg Pnum < deg V), at level
; L = K+1.  Residues are the rational roots of res_theta(V, Pnum - c V'); for each rational root c the log
; argument is gcd_theta(V, Pnum - c V').  Returns (list 'rootsum ((c arg) ...)) | (list 'algebraic) | (list 'none).
(define (rtl-logpart K specs spec Pnum V)
  (let ((DV (rtp-deriv K spec specs V)))
    (let ((N (rtp-deg K V)))
      (let ((rs (rtl-resvals K specs spec Pnum V DV 0 N (quote ()))))
        (let ((k0 (rtl-first-nonzero rs 0)))
          (if (< k0 0) (list (quote none))
              (let ((rats (rtl-ratios K rs (rtl-nth rs k0) (quote ()))))
                (if (equal? rats (quote notconst)) (list (quote algebraic))
                    (let ((roots (ros-rational-roots (qz-lagrange (rtl-intlist 0 N) rats))))
                      (let ((terms (rtl-terms K specs spec Pnum V DV roots (quote ()))))
                        ; completeness: the log arguments must account for the ENTIRE squarefree denominator.
                        ; if the residue-argument degrees sum to deg V the RootSum is complete (elementary over Q);
                        ; otherwise some residues are algebraic (e.g. complex), so the integral is NOT elementary.
                        (if (= (rtl-degsum K terms 0) N) (list (quote rootsum) terms) (list (quote algebraic)))))))))))))
(define (rtl-degsum K terms acc) (if (null? terms) acc (rtl-degsum K (cdr terms) (+ acc (rtp-deg K (car (cdr (car terms))))))))
; evaluate res(V, Pnum - k V') at integer k = 0..N (each a level-K field element)
(define (rtl-resvals K specs spec Pnum V DV k N acc)
  (if (> k N) (reverse acc)
      (let ((rv (rtp-resultant K V (rtp-sub K Pnum (rtp-cscale K (rt-from-int K k) DV)))))
        (begin (gc) (rtl-resvals K specs spec Pnum V DV (+ k 1) N (cons rv acc))))))
(define (rtl-first-nonzero rs k) (if (null? rs) -1 (if (rt-zero?-elt (car rs)) (rtl-first-nonzero (cdr rs) (+ k 1)) k)))
; a resultant value lives at the level-K field; it is zero iff its numerator fp is zero (denominator nonzero)
(define (rt-zero?-elt a) (if (null? a) #t (rtp-zero?-base (car a))))
(define (rtp-zero?-base p) (rtl-all-rat-zero p))
(define (rtl-all-rat-zero p) (if (null? p) #t (if (rtl-eltzero (car p)) (rtl-all-rat-zero (cdr p)) #f)))
(define (rtl-eltzero c) (if (null? c) #t (if (= (length c) 2) (poly-zero? (rtl-numpart c)) #f)))
(define (rtl-numpart c) (car c))
(define (rtl-nth lst i) (if (= i 0) (car lst) (rtl-nth (cdr lst) (- i 1))))
(define (rtl-intlist k n) (if (> k n) (quote ()) (cons k (rtl-intlist (+ k 1) n))))
; ratios res(k)/res(k0): a genuine RootSum has these ratios equal to RATIONAL CONSTANTS; collect or 'notconst
(define (rtl-ratios K rs r0 acc)
  (if (null? rs) (reverse acc)
      (let ((s (rt-div K (car rs) r0)))
        (if (rtl-const? K s) (rtl-ratios K (cdr rs) r0 (cons (rtl-to-rat K s) acc)) (quote notconst)))))
; a level-K element is a rational constant iff it equals its base-rational and that rational is constant in x.
; We descend to the base rat and require numerator/denominator to be degree-0 polynomials.
(define (rtl-base-rat K a) (if (= K 0) (rt-r a) (rtl-base-rat (- K 1) (rtl-lead-coeff a))))
(define (rtl-lead-coeff a) (rtl-c0 (car a)))                     ; theta^0 coefficient of the numerator fp
(define (rtl-c0 p) (if (null? p) (quote ()) (car p)))
(define (rtl-const? K a)
  (let ((deg-ok (rtl-isconst-shape K a)))
    (if deg-ok (let ((r (rtl-base-rat K a))) (and (rtl-deg0? (rat-num (rt-r r))) (rtl-deg0? (rat-den (rt-r r))))) #f)))
(define (rtl-deg0? p) (<= (poly-deg p) 0))
; shape check: every theta-coefficient above degree 0 vanishes, recursively, so a is a lifted base element
(define (rtl-isconst-shape K a)
  (if (= K 0) #t
      (and (rtl-deg<=0 (car a)) (rtl-deg<=0 (car (cdr a))) (rtl-isconst-shape (- K 1) (rtl-lead-coeff a)))))
(define (rtl-deg<=0 p) (<= (rtl-fpdeg p) 0))
(define (rtl-fpdeg p) (- (length (rtl-fpnorm p)) 1))
(define (rtl-fpnorm p) (reverse (rtl-fpdrop (reverse p))))
(define (rtl-fpdrop p) (if (null? p) (quote ()) (if (rtl-eltzero-deep (car p)) (rtl-fpdrop (cdr p)) p)))
(define (rtl-eltzero-deep c) (if (null? c) #t (if (= (length c) 2) (poly-zero? (rtl-numpart c)) #f)))
(define (rtl-to-rat K a) (let ((r (rtl-base-rat K a))) (/ (poly-coeff (rat-num (rt-r r)) 0) (poly-coeff (rat-den (rt-r r)) 0))))
; build (c arg) for each rational residue c with deg(arg) >= 1
(define (rtl-terms K specs spec Pnum V DV roots acc)
  (if (null? roots) (reverse acc)
      (let ((c (car roots)))
        (let ((v (rtp-monic K (rtp-gcd K V (rtp-sub K Pnum (rtp-cscale K (rt-from-rat K c) DV))))))
          (if (>= (rtp-deg K v) 1)
              (rtl-terms K specs spec Pnum V DV (cdr roots) (cons (list c v) acc))
              (rtl-terms K specs spec Pnum V DV (cdr roots) acc))))))
(define (rt-from-rat K c) (if (= K 0) (rat-make (list (numerator c)) (list (denominator c))) (list (rtp-lift (- K 1) (rt-from-rat (- K 1) c)) (rtp-one (- K 1)))))

; ----- certificate for a RootSum logarithmic part: sum_i c_i (D v_i)/v_i = Pnum/V -----
; cleared by V (= prod v_i, squarefree): sum_i c_i (D v_i)(V/v_i) = Pnum, checked over the level-K field.
; V/v_i is computed as the product of the OTHER arguments (no division), keeping coefficients small.
(define (rtl-logpart-verify K specs spec Pnum V res)
  (if (not (equal? (car res) (quote rootsum))) #f
      (let ((terms (car (cdr res))))
        (let ((Vp (rtl-prod-args K terms)))           ; product of all v_i (should match V up to the squarefree part)
          (let ((lhs (rtl-logsum K specs spec terms 0)))
            ; identity over the product Vp:  sum_i c_i (D v_i)(Vp/v_i) = Pnum * (Vp / V).  Since V is squarefree
            ; and equals prod v_i for a complete RootSum, Vp = V, so check sum = Pnum directly.
            (rtp-zero? K (rtp-sub K lhs (rtp-mul K Pnum (rtp-div K Vp V)))))))))
(define (rtl-prod-args K terms) (if (null? terms) (rtp-one K) (rtp-mul K (car (cdr (car terms))) (rtl-prod-args K (cdr terms)))))
; sum_i c_i (D v_i) * (product of other args).  We keep the FULL terms list and walk an absolute index i,
; so "other args" always ranges over every term except i (the earlier cdr-recursion bug dropped earlier terms).
(define (rtl-logsum K specs spec terms i)
  (rtl-logsum-go K specs spec terms terms i 0))
(define (rtl-logsum-go K specs spec all terms base j)
  (if (null? terms) (rtp-zero)
      (let ((c (car (car terms))) (v (car (cdr (car terms)))))
        (rtp-add K (rtp-cscale K (rt-from-rat K c) (rtp-mul K (rtp-deriv K spec specs v) (rtl-others K all j 0)))
                   (rtl-logsum-go K specs spec all (cdr terms) base (+ j 1))))))
(define (rtl-others K terms i j)
  (if (null? terms) (rtp-one K)
      (if (= j i) (rtl-others K (cdr terms) i (+ j 1))
          (rtp-mul K (car (cdr (car terms))) (rtl-others K (cdr terms) i (+ j 1))))))

; ===================== top-level proper-fraction integrator with multi-residue logs =====================
; INT (Pnum / V) dx at level L = K+1, V monic squarefree in theta_L, deg Pnum < deg V.
; Returns (list 'elementary 'rootsum terms) where terms = ((c_i v_i) ...) meaning sum c_i log(v_i),
;        | (list 'algebraic) | (list 'none) | (list 'non-elementary why).
; Multi-residue, at arbitrary depth, certified by rtl-logpart-verify.
(define (rt-integrate-logpart L specs Pnum V)
  (let ((K (- L 1)) (spec (rt-nth specs (- L 1))))
    (let ((res (rtl-logpart K specs spec Pnum V)))
      (cond ((equal? (car res) (quote rootsum))
             (if (rtl-logpart-verify K specs spec Pnum V res) (list (quote elementary) (quote rootsum) (car (cdr res)))
                 (list (quote non-elementary) "RootSum candidate failed its certificate")))
            ((equal? (car res) (quote algebraic)) (list (quote non-elementary) "logarithmic part has algebraic (non-rational) residues"))
            (else (list (quote none)))))))
(define (rt-integrate-logpart-decides? L specs Pnum V)
  (let ((r (rt-integrate-logpart L specs Pnum V))) (equal? (car r) (quote elementary)))) 
