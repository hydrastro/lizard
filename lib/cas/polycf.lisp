; -*- lisp -*-
; lib/cas/polycf.lisp -- the CONTINUED FRACTION of sqrt(f) over Q[x], with periodicity detection and the
; fundamental-unit convergent: the function-field analogue of the numeric continued fraction for sqrt(N), the
; engine that decides whether a hyperelliptic curve y^2 = f has a Pell unit and produces it (docs/TRAGER_ROADMAP.md
; -- generalizing the f = h^2 + c family of hyperpell to arbitrary curves whose sqrt(f) is periodic).
;
; For f of even degree 2d, sqrt(f) has a polynomial part a0 (the unique polynomial s of degree d with s^2 agreeing
; with f in its top d+1 coefficients).  Abel's algorithm expands sqrt(f) as a continued fraction exactly as the
; numeric algorithm expands sqrt(N): with complete quotients (P_i + sqrt(f))/Q_i represented by the polynomial
; pair (P_i, Q_i),
;     a_i = polypart((P_i + a0)/Q_i)   (the polynomial quotient),
;     P_{i+1} = a_i Q_i - P_i,
;     Q_{i+1} = (f - P_{i+1}^2)/Q_i    (an exact polynomial division),
; started from P_0 = 0, Q_0 = 1, a_0 = polypart(sqrt f).  The expansion is PERIODIC exactly when some Q_i returns
; to a nonzero CONSTANT (degree 0); the curve y^2 = f then has a fundamental Pell unit, and the convergent h_{k}/
; k_{k} at the end of the first period (the step before Q becomes constant) gives a unit A + B sqrt(f) with
; A^2 - B^2 f a nonzero constant.  When no Q_i becomes constant within a search bound the CF is reported aperiodic-
; up-to-that-bound (no unit found), never a false "no unit": the unconditional reducedness/periodicity bound for a
; given genus needs deeper theory and is out of scope, so the negative is always bounded.
;
; The convergents are built by the standard recurrence h_i = a_i h_{i-1} + h_{i-2}, k_i = a_i k_{i-1} + k_{i-2}
; over Q[x]; at the period the pair (h_{period-1}, k_{period-1}) is the fundamental unit (A, B), checked against
; the norm A^2 - B^2 f being a nonzero constant.  Everything is exact over Q[x] and certified by that norm.
;
; Public (f a polynomial of even degree; B a positive search bound on the period length):
;   pcf-polypart f             -> the polynomial part a0 of sqrt(f) (degree deg(f)/2), or 'not-even-degree
;   pcf-step f P Q             -> one Abel step from (P, Q): (list a Pnext Qnext)
;   pcf-quotients f B          -> the list (a0 a1 ... a_{period-1}) up to the period, or up to bound B
;   pcf-period f B             -> the period length (steps until Q is constant), or (list 'aperiodic-up-to B)
;   pcf-is-periodic? f B       -> #t iff sqrt(f) is periodic within bound B (the curve has a Pell unit)
;   pcf-fundamental-unit f B   -> (list A B-poly) the convergent unit at the period, or 'no-unit-up-to B
;   pcf-unit-norm f A Bp       -> A^2 - Bp^2 f (the unit norm; a nonzero constant for a genuine unit)
;   pcf-certify-unit f B       -> #t iff the fundamental unit found has constant nonzero norm (the Pell certificate)
;
; Verified: f = x^6 + 1 has a0 = x^3, Q1 = 1 (period 1), and the fundamental unit (x^3, 1) with norm -1 (matching
; hyperpell); f = x^6 + x^2 + 1 has a0 = x^3, Q1 = x^2 + 1 (not period 1), and the CF continues; a deg-2 f = x^2 + 1
; (genus 0) recovers the classical unit; the period and unit norm certify.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (pcf-len l) (if (null? l) 0 (+ 1 (pcf-len (cdr l)))))
(define (pcf-nth l k) (if (= k 0) (car l) (pcf-nth (cdr l) (- k 1))))
(define (pcf-deg p) (- (pcf-trim p) 1))
(define (pcf-trim p) (pcf-trim-n p (pcf-len p)))
(define (pcf-trim-n p k) (cond ((= k 0) 0) ((= (pcf-coef p (- k 1)) 0) (pcf-trim-n p (- k 1))) (else k)))
(define (pcf-coef p k) (if (< k (pcf-len p)) (pcf-nth p k) 0))
(define (poly-norm p) (reverse (pcf-drop0 (reverse p))))
(define (pcf-drop0 p) (cond ((null? p) (quote ())) ((= (car p) 0) (pcf-drop0 (cdr p))) (else p)))
(define (pcf-const? p) (<= (pcf-deg p) 0))
(define (pcf-zero? p) (null? (poly-norm p)))

; ----- polypart(sqrt f): polynomial s of degree d = deg(f)/2 with s^2 matching f's top d+1 coeffs -----
(define (pcf-polypart f) (pcf-pp-dispatch (poly-norm f)))
(define (pcf-pp-dispatch f)
  (if (= (remainder (pcf-deg f) 2) 1) (quote not-even-degree)
      (pcf-pp-build f (quotient (pcf-deg f) 2))))
(define (pcf-pp-build f d) (pcf-pp-lead f d (pcf-rat-sqrt (pcf-coef f (* 2 d)))))
(define (pcf-pp-lead f d sl) (if (equal? sl (quote not-square)) (quote not-square) (pcf-pp-coeffs f d sl)))
; build s top-down: s_d = sl; s_{d-j} from matching coefficient of x^(2d-j) in s^2
(define (pcf-pp-coeffs f d sl) (pcf-pp-fill f d sl (list (cons d sl)) 1))
(define (pcf-pp-fill f d sl known j)
  (if (> j d) (pcf-pp-assemble known d)
      (pcf-pp-fill f d sl (cons (cons (- d j) (pcf-pp-solve f d sl known j)) known) (+ j 1))))
; coefficient of x^(2d-j) in s^2 = sum_{p+q=2d-j} s_p s_q ; isolate 2 sl s_{d-j}
(define (pcf-pp-solve f d sl known j) (/ (- (pcf-coef f (- (* 2 d) j)) (pcf-pp-cross known d j)) (* 2 sl)))
(define (pcf-pp-cross known d j) (pcf-pp-cross-go known d (- (* 2 d) j) 0))
; sum s_p s_q with p+q = idx, p>q (each unordered pair once, times 2), over already-known coeffs > d-j
(define (pcf-pp-cross-go known d idx acc) (pcf-ppc known d idx (- d 1) acc))
(define (pcf-ppc known d idx p acc)
  (cond ((<= p (quotient idx 2)) acc)                          ; stop at midpoint (p>q)
        (else (pcf-ppc known d idx (- p 1) (+ acc (* 2 (* (pcf-klook known p) (pcf-klook known (- idx p)))))))))
(define (pcf-klook known k) (cond ((null? known) 0) ((= (car (car known)) k) (cdr (car known))) (else (pcf-klook (cdr known) k))))
(define (pcf-pp-assemble known d) (pcf-ppa known 0 d (quote ())))
(define (pcf-ppa known i d acc) (if (> i d) (reverse acc) (pcf-ppa known (+ i 1) d (cons (pcf-klook known i) acc))))

(define (pcf-rat-sqrt c) (if (< c 0) (quote not-square) (pcf-rs (pcf-isqrt (numerator c)) (pcf-isqrt (denominator c)))))
(define (pcf-rs ns ds) (if (equal? ns (quote not-square)) (quote not-square) (if (equal? ds (quote not-square)) (quote not-square) (/ ns ds))))
(define (pcf-isqrt n) (pcf-is n 0))
(define (pcf-is n k) (cond ((> (* k k) n) (quote not-square)) ((= (* k k) n) k) (else (pcf-is n (+ k 1)))))

; ----- one Abel step from (P, Q): a = polypart((P + a0)/Q), Pnext = a Q - P, Qnext = (f - Pnext^2)/Q -----
(define (pcf-step f P Q) (pcf-step-go f P Q (pcf-polypart f)))
(define (pcf-step-go f P Q a0) (pcf-step-a f P Q (car (poly-divmod (poly-add P a0) Q))))
(define (pcf-step-a f P Q a) (pcf-step-fin f P Q a (poly-sub (poly-mul a Q) P)))
(define (pcf-step-fin f P Q a Pn) (list a Pn (car (poly-divmod (poly-sub f (poly-mul Pn Pn)) Q))))

; ----- the quotient list up to the period (Q constant) or bound B -----
; the iteration advances a_1, a_2, ... from the state AFTER a_0: P_1 = a_0, Q_1 = (f - a_0^2)/Q_0 = f - a_0^2.
(define (pcf-quotients f B) (pcf-q-start f (pcf-polypart f) B))
(define (pcf-q-start f a0 B) (pcf-q-go f a0 (poly-sub f (poly-mul a0 a0)) (list a0) 1 B))
(define (pcf-q-go f P Q acc i B)
  (if (pcf-const? Q) (reverse acc)                             ; Q already constant (period 1: a0 alone)
      (if (> i B) (reverse acc)
          (pcf-q-dispatch f (pcf-step f P Q) acc i B))))
(define (pcf-q-dispatch f st acc i B)
  (if (pcf-const? (pcf-nth st 2))                              ; Qnext constant -> period reached, include this a then stop
      (reverse (cons (car st) acc))
      (pcf-q-go f (pcf-nth st 1) (pcf-nth st 2) (cons (car st) acc) (+ i 1) B)))

; ----- period length: number of steps until Q becomes constant (a0 is step 0; period 1 = a0 with Q1 constant) -----
(define (pcf-period f B) (pcf-per-start f (pcf-polypart f) B))
(define (pcf-per-start f a0 B) (pcf-per-go f a0 (poly-sub f (poly-mul a0 a0)) 1 B))
(define (pcf-per-go f P Q i B)
  (if (pcf-const? Q) i                                          ; Q_i constant -> period is i
      (if (>= i B) (list (quote aperiodic-up-to) B)
          (pcf-per-step f (pcf-step f P Q) i B))))
(define (pcf-per-step f st i B)
  (if (pcf-const? (pcf-nth st 2)) (+ i 1)
      (pcf-per-go f (pcf-nth st 1) (pcf-nth st 2) (+ i 1) B)))
(define (pcf-is-periodic? f B) (not (pcf-aperiodic? (pcf-period f B))))
(define (pcf-aperiodic? r) (if (pair? r) #t #f))                ; aperiodic verdict is a list

; ----- convergents h_i, k_i and the fundamental unit at the period -----
; the unit is (h_{L-1}, k_{L-1}) where L is the period length; the convergent uses the FIRST L quotients
; (a_0 .. a_{L-1}).  pcf-quotients returns (a_0 .. a_L) including the period-closing quotient, so we take the
; first L of them (drop the last) before building the convergent.
(define (pcf-fundamental-unit f B) (pcf-fu-dispatch f (pcf-period f B) B))
(define (pcf-fu-dispatch f L B) (if (pcf-aperiodic? L) (quote no-unit-up-to) (pcf-fu-build f (pcf-take (pcf-quotients f B) L))))
(define (pcf-take l n) (if (<= n 0) (quote ()) (if (null? l) (quote ()) (cons (car l) (pcf-take (cdr l) (- n 1))))))
; build convergent from the quotient list (a0 .. a_{L-1}); unit = (h_{L-1}, k_{L-1})
(define (pcf-fu-build f qs) (pcf-conv qs))
(define (pcf-conv qs) (pcf-conv-go qs (list 1) (list 0) (list 0) (list 1)))   ; h_{-1}=1,h_{-2}=0 ; k_{-1}=0,k_{-2}=1
(define (pcf-conv-go qs hm1 hm2 km1 km2)
  (if (null? qs) (list hm1 km1)
      (pcf-conv-go (cdr qs)
                   (poly-add (poly-mul (car qs) hm1) hm2) hm1
                   (poly-add (poly-mul (car qs) km1) km2) km1)))

; ----- unit norm and certificate -----
(define (pcf-unit-norm f A Bp) (poly-sub (poly-mul A A) (poly-mul (poly-mul Bp Bp) f)))
(define (pcf-certify-unit f B) (pcf-cert-dispatch f (pcf-fundamental-unit f B)))
(define (pcf-cert-dispatch f u) (if (equal? u (quote no-unit-up-to)) #f (pcf-cert-check f (pcf-unit-norm f (car u) (car (cdr u))))))
(define (pcf-cert-check f N) (if (pcf-const? N) (not (pcf-zero? N)) #f))

; ----- verified fundamental unit: return the unit only when its norm is a nonzero constant (certified) -----
; periodicity detection finds a candidate; this returns it only if the Pell certificate holds, else 'unit-unverified
; (the period-1 family -- including f = h^2 + c -- is fully certified; higher even periods, where the true unit is
; a higher convergent, return unit-unverified rather than a wrong unit -- sound, never asserted).
(define (pcf-unit-verified f B) (pcf-uv-dispatch f (pcf-fundamental-unit f B) B))
(define (pcf-uv-dispatch f u B) (if (equal? u (quote no-unit-up-to)) (quote no-unit-up-to) (pcf-uv-check f u)))
(define (pcf-uv-check f u) (if (pcf-cert-check f (pcf-unit-norm f (car u) (car (cdr u)))) u (quote unit-unverified)))

; ----- perfect-square guard and explicit unit status -----
; a perfect-square f has sqrt(f) polynomial (no quadratic irrational, no Pell unit); flag it explicitly rather
; than returning a degenerate unit-unverified.
(define (pcf-is-square? f) (pcf-sq-check f (pcf-polypart f)))
(define (pcf-sq-check f pp) (if (equal? pp (quote not-even-degree)) #f (equal? (poly-norm (poly-mul pp pp)) (poly-norm f))))
; the full classification of a curve's Pell status within bound B:
;   'square          : f is a perfect square (sqrt polynomial, no unit)
;   (list 'unit A B)  : a certified fundamental unit A + B y with constant nonzero norm
;   'unit-unverified  : periodic but the convergent did not yield a constant-norm unit (deferred)
;   'no-unit-up-to    : aperiodic within the search bound
(define (pcf-unit-status f B) (if (pcf-is-square? f) (quote square) (pcf-status-go f (pcf-unit-verified f B))))
(define (pcf-status-go f u) (if (pair? u) (if (pair? (car u)) (list (quote unit) (car u) (car (cdr u))) (quote unit-unverified)) u))
