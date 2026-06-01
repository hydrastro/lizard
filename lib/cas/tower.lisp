; -*- lisp -*-
; lib/cas/tower.lisp — rational functions of a single transcendental monomial
; theta (= exp(u) or log(x)) over Q(x), with the EXACT differential-field
; derivation, and certified integration of two genuinely-new families:
;   * new logarithms:  INT f = log(g)  when f = D(g)/g   (e.g. 1/(x log x) = log log x)
;   * negative powers (Hermite rational part of a primitive monomial):
;       INT (c/x) (log x)^(-k) = -c/(k-1) (log x)^(1-k)  (e.g. 1/(x (log x)^2) = -1/log x)
;
; The key upgrade over risch.lisp is that coefficients now live in Q(x), so the
; derivation handles D(log x) = 1/x exactly.  A "tower polynomial" (rfpoly) is a
; list of Q(x) coefficients low-to-high in theta; a "tower rational function" is
; a pair (N D) of rfpolys.  Every result is CERTIFIED: for a rational answer by
; differentiating it back through the field (tr-equal? with the integrand), and
; for a new log by the defining identity D(g)/g = f, which is the check itself.
;
; Top-level helpers only; builds on ratfun.lisp (Q(x)) and risch.lisp (poly RDE).

(import "cas/ratfun.lisp")
(import "cas/risch.lisp")

; ---- coefficient field K = Q(x): the operations ratfun.lisp doesn't expose ----
(define (rat-zero) (list '() (list 1)))
(define (rat-one) (list (list 1) (list 1)))
(define (rat-zero? r) (poly-zero? (rat-num r)))
(define (rat-neg r) (list (poly-neg (rat-num r)) (rat-den r)))
(define (rat-sub a b) (rat-add a (rat-neg b)))
(define (rat-inv r) (rat-make (rat-den r) (rat-num r)))
(define (rat-div a b) (rat-mul a (rat-inv b)))
(define (rat-from-poly p) (rat-make p (list 1)))
(define (rat-scale c r) (list (poly-scale c (rat-num r)) (rat-den r)))        ; c a rational number
(define (rat-deriv r)                                                          ; (n/d)' = (n'd - n d')/d^2
  (rat-make (poly-sub (poly-mul (poly-deriv (rat-num r)) (rat-den r))
                      (poly-mul (rat-num r) (poly-deriv (rat-den r))))
            (poly-mul (rat-den r) (rat-den r))))
(define (ratf->string r)
  (if (equal? (rat-den r) (list 1)) (poly->string (rat-num r) "x")
    (string-append "(" (poly->string (rat-num r) "x") ")/(" (poly->string (rat-den r) "x") ")")))

; ============================================================
;  rfpoly: polynomials in theta with Q(x) coefficients (low-to-high)
; ============================================================
(define (rat-zeros n) (if (= n 0) '() (cons (rat-zero) (rat-zeros (- n 1)))))
(define (rf-trim rc) (cond ((null? rc) '()) ((rat-zero? (car rc)) (rf-trim (cdr rc))) (else rc)))
(define (rfpoly-norm P) (reverse (rf-trim (reverse P))))
(define (rfpoly-zero? P) (null? (rfpoly-norm P)))
(define (rfpoly-deg P) (- (length (rfpoly-norm P)) 1))
(define (rfpoly-lead P) (car (reverse (rfpoly-norm P))))
(define (rfpoly-equal? A B) (equal? (rfpoly-norm A) (rfpoly-norm B)))
(define (rfpoly-add A B) (cond ((null? A) B) ((null? B) A) (else (cons (rat-add (car A) (car B)) (rfpoly-add (cdr A) (cdr B))))))
(define (rfpoly-neg P) (map rat-neg P))
(define (rfpoly-sub A B) (rfpoly-add A (rfpoly-neg B)))
(define (rfpoly-cscale c P) (map (lambda (x) (rat-mul c x)) P))               ; scale by a Q(x) coefficient
(define (rfpoly-mul A B)
  (if (or (null? A) (null? B)) '()
    (rfpoly-add (rfpoly-cscale (car A) B) (cons (rat-zero) (rfpoly-mul (cdr A) B)))))
(define (rfpoly-monomial c k) (append (rat-zeros k) (list c)))
(define (rfpoly-monic P) (if (rfpoly-zero? P) P (rfpoly-cscale (rat-inv (rfpoly-lead P)) P)))
(define (rf-const r) (list r))                                                ; degree-0 rfpoly
(define (rf-theta) (list (rat-zero) (rat-one)))                               ; theta
(define (rf-from-polys ps) (map rat-from-poly ps))                            ; Q[x] coeffs -> Q(x) coeffs

(define (rfpoly->string P th)
  (let ((n (rfpoly-norm P))) (if (null? n) "0" (rf-terms (reverse n) (- (length n) 1) th #t))))
(define (rf-terms rc deg th first)
  (if (null? rc) (if first "0" "")
    (if (rat-zero? (car rc)) (rf-terms (cdr rc) (- deg 1) th first)
      (string-append (if first "" " + ") "(" (ratf->string (car rc)) ")"
                     (cond ((= deg 0) "") ((= deg 1) (string-append "*" th)) (else (string-append "*" th "^" (number->string deg))))
                     (rf-terms (cdr rc) (- deg 1) th #f)))))

; ============================================================
;  the differential-field derivation D
; ============================================================
(define (Drf-exp-terms P i up)             ; theta = exp(u): D(sum a_i th^i) = sum (a_i' + i u' a_i) th^i
  (if (null? P) '()
    (cons (rat-add (rat-deriv (car P)) (rat-mul (rat-scale i up) (car P)))
          (Drf-exp-terms (cdr P) (+ i 1) up))))
(define (Drf-log-terms P i tp)             ; theta = log(x), tp = 1/x: + (i+1) a_{i+1} (1/x)
  (if (null? P) '()
    (cons (rat-add (rat-deriv (car P)) (if (null? (cdr P)) (rat-zero) (rat-scale (+ i 1) (rat-mul tp (car (cdr P))))))
          (Drf-log-terms (cdr P) (+ i 1) tp))))
(define (Drf P mono)
  (if (equal? (car mono) 'exp) (Drf-exp-terms P 0 (rat-from-poly (poly-deriv (car (cdr mono)))))
    (Drf-log-terms P 0 (rat-make (list 1) (list 0 1)))))

; ============================================================
;  tower rational functions  tr = (list N D)   (= N/D),  with D from the field
; ============================================================
(define (tr-make N D) (list N D))
(define (tr-deriv tr mono)                 ; (N/D)' = (D(N) D - N D(D)) / D^2
  (let ((N (car tr)) (Dl (car (cdr tr))))
    (list (rfpoly-sub (rfpoly-mul (Drf N mono) Dl) (rfpoly-mul N (Drf Dl mono))) (rfpoly-mul Dl Dl))))
(define (tr-equal? a b)                    ; N1/D1 == N2/D2  <=>  N1 D2 == N2 D1
  (rfpoly-equal? (rfpoly-mul (car a) (car (cdr b))) (rfpoly-mul (car b) (car (cdr a)))))
(define (tr->string tr th)
  (if (rfpoly-equal? (car (cdr tr)) (rf-const (rat-one))) (rfpoly->string (car tr) th)
    (string-append "(" (rfpoly->string (car tr) th) ") / (" (rfpoly->string (car (cdr tr)) th) ")")))

; ============================================================
;  integration toolbox (each result certified)
; ============================================================
; (1) new logarithm:  f = D(g)/g  ->  INT f = log(g)
(define (newlog-check? f g mono)           ; f = N/D ; check N*g == D * D(g)
  (and (not (rfpoly-zero? g))
       (rfpoly-equal? (rfpoly-mul (car f) g) (rfpoly-mul (car (cdr f)) (Drf g mono)))))
(define (try-newlog f mono)
  (newlog-scan f mono (list (car (cdr f)) (rf-theta))))
(define (newlog-scan f mono cands)
  (cond ((null? cands) 'none)
        ((newlog-check? f (car cands) mono) (list 'log (car cands)))
        (else (newlog-scan f mono (cdr cands)))))

; (2) Hermite rational part for a primitive monomial:  (c/x) (log x)^(-k), k>=2
;     -> -c/(k-1) (log x)^(1-k).  Detect D = theta^k and N a degree-0 (c/x).
(define (pure-power-deg D)                 ; if D = theta^k (monic), return k, else -1
  (let ((n (rfpoly-norm D)))
    (if (and (> (length n) 0) (all-zero-but-top n)) (- (length n) 1) -1)))
(define (all-zero-but-top n)               ; every coeff except the last is zero, last is 1
  (cond ((null? (cdr n)) (equal? (car n) (rat-one)))
        ((rat-zero? (car n)) (all-zero-but-top (cdr n)))
        (else #f)))
(define (try-logpow f mono)
  (if (not (equal? (car mono) 'log)) 'none
    (let ((k (pure-power-deg (car (cdr f)))) (N (rfpoly-norm (car f))))
      (if (and (>= k 2) (= (length N) 1))                    ; N is degree 0 in theta: a coefficient c0(x)
          (let ((c0 (car N)) (xinv (rat-make (list 1) (list 0 1))))
            ; need c0 == c * (1/x)  i.e. c = c0 * x
            (let ((c (rat-mul c0 (rat-make (list 0 1) (list 1)))))   ; c = c0 * x
              (list 'rat (tr-make (rf-const (rat-neg (rat-scale (/ 1 (- k 1)) c)))
                                  (rfpoly-monomial (rat-one) (- k 1))))))
          'none))))

; top-level: integrate a tower rational function; returns
;   (list 'log g) | (list 'rat tr) | (list 'failed)   -- the first two are certified
(define (integrate-tower f mono)
  (let ((nl (try-newlog f mono)))
    (if (not (equal? nl 'none))
        (if (newlog-check? f (car (cdr nl)) mono) nl (list 'failed))
      (let ((lp (try-logpow f mono)))
        (if (not (equal? lp 'none))
            (if (tr-equal? (tr-deriv (car (cdr lp)) mono) f) lp (list 'failed))
          (list 'failed))))))

(define (tower-result->string r mono)
  (cond ((equal? (car r) 'log) (string-append "log(" (rfpoly->string (car (cdr r)) (if (equal? (car mono) 'exp) "E" "L")) ")"))
        ((equal? (car r) 'rat) (tr->string (car (cdr r)) (if (equal? (car mono) 'exp) "E" "L")))
        (else "not resolved by this toolbox")))

; ============================================================
;  general Hermite reduction over a primitive monomial
;  (squarefree factorization + extended Euclid in Q(x)[theta])
; ============================================================
(define (rfpoly-dtheta-terms P i) (if (null? P) '() (cons (rat-scale i (car P)) (rfpoly-dtheta-terms (cdr P) (+ i 1)))))
(define (rfpoly-dtheta P) (if (or (null? P) (null? (cdr P))) '() (rfpoly-dtheta-terms (cdr P) 1)))   ; d/dtheta
(define (rfpoly-divmod-loop r d q)
  (if (< (rfpoly-deg r) (rfpoly-deg d)) (list (rfpoly-norm q) (rfpoly-norm r))
    (let ((c (rat-div (rfpoly-lead r) (rfpoly-lead d))) (k (- (rfpoly-deg r) (rfpoly-deg d))))
      (let ((t (rfpoly-monomial c k)))
        (rfpoly-divmod-loop (rfpoly-sub r (rfpoly-mul t d)) d (rfpoly-add q t))))))
(define (rfpoly-divmod a b) (rfpoly-divmod-loop (rfpoly-norm a) (rfpoly-norm b) '()))
(define (rfpoly-div a b) (car (rfpoly-divmod a b)))
(define (rfpoly-rem a b) (car (cdr (rfpoly-divmod a b))))
(define (rfpoly-gcd a b) (if (rfpoly-zero? b) (rfpoly-monic a) (rfpoly-gcd b (rfpoly-rem a b))))
(define (rfpoly-pow P k) (if (= k 0) (rf-const (rat-one)) (rfpoly-mul P (rfpoly-pow P (- k 1)))))
(define (rf-iscale i P) (rfpoly-cscale (rat-from-poly (list i)) P))

; extended Euclid: returns (gcd s t) with s*a + t*b = gcd
(define (rf-eea or r os s ot t)
  (if (rfpoly-zero? r) (list or os ot)
    (let ((q (rfpoly-div or r)))
      (rf-eea r (rfpoly-sub or (rfpoly-mul q r)) s (rfpoly-sub os (rfpoly-mul q s)) t (rfpoly-sub ot (rfpoly-mul q t))))))
(define (rfpoly-bezout a b) (rf-eea a b (rf-const (rat-one)) '() '() (rf-const (rat-one))))
(define (rfpoly-invmod p v)                  ; inverse of p modulo v (gcd(p,v)=1)
  (let ((res (rfpoly-bezout (rfpoly-rem p v) v)))
    (rfpoly-rem (rfpoly-cscale (rat-inv (car (rfpoly-norm (car res)))) (car (cdr res))) v)))

; Yun squarefree factorization w.r.t theta: list of (mult . squarefree-factor)
(define (rf-yun f)
  (let ((a (rfpoly-gcd f (rfpoly-dtheta f))))
    (let ((b (rfpoly-div f a)) (c (rfpoly-div (rfpoly-dtheta f) a)))
      (rf-yun-loop b (rfpoly-sub c (rfpoly-dtheta b)) 1 '()))))
(define (rf-yun-loop b d i acc)
  (if (<= (rfpoly-deg b) 0) (reverse acc)
    (let ((g (rfpoly-gcd b d)))
      (let ((bn (rfpoly-div b g)) (cn (rfpoly-div d g)))
        (rf-yun-loop bn (rfpoly-sub cn (rfpoly-dtheta bn)) (+ i 1)
                     (if (> (rfpoly-deg g) 0) (cons (cons i (rfpoly-monic g)) acc) acc))))))
(define (rf-max-mult sf best) (if (null? sf) best (rf-max-mult (cdr sf) (if (> (car (car sf)) (car best)) (car sf) best))))

; tower-rational zero/add (for accumulating the rational part)
(define (tr-zero) (list '() (rf-const (rat-one))))
(define (tr-add a b) (list (rfpoly-add (rfpoly-mul (car a) (car (cdr b))) (rfpoly-mul (car b) (car (cdr a))))
                           (rfpoly-mul (car (cdr a)) (car (cdr b)))))
(define (tr-reduce tr)                       ; lowest terms, denominator monic in theta
  (let ((N (car tr)) (Dn (car (cdr tr))))
    (if (rfpoly-zero? N) (tr-zero)
      (let ((g (rfpoly-gcd N Dn)))
        (let ((Dr (rfpoly-div Dn g)))
          (let ((lc (rat-inv (rfpoly-lead Dr))))
            (list (rfpoly-cscale lc (rfpoly-div N g)) (rfpoly-cscale lc Dr))))))))

; one Hermite step: reduce factor v (multiplicity m>=2) in a/d by one.
; returns (list g-part a' d') with  a/d = D(g-part) + a'/d',  d' = v^(m-1) * w
(define (hermite-step a d m v mono)
  (let ((w (rfpoly-div d (rfpoly-pow v m))) (Dv (Drf v mono)))
    (let ((b (rfpoly-rem (rfpoly-mul (rfpoly-neg a)
                                     (rfpoly-invmod (rf-iscale (- m 1) (rfpoly-mul w Dv)) v)) v)))
      (let ((Db (Drf b mono)))
        (let ((num (rfpoly-add a (rfpoly-add (rfpoly-neg (rfpoly-mul w (rfpoly-mul Db v)))
                                             (rf-iscale (- m 1) (rfpoly-mul w (rfpoly-mul b Dv)))))))
          (list (list b (rfpoly-pow v (- m 1)))            ; g-part = b / v^(m-1)
                (rfpoly-div num v)
                (rfpoly-mul (rfpoly-pow v (- m 1)) w)))))))

(define (hermite-loop a d g mono)
  (let ((hi (rf-max-mult (rf-yun d) (cons 0 '()))))
    (if (<= (car hi) 1) (list g a d)
      (let ((step (hermite-step a d (car hi) (cdr hi) mono)))
        (hermite-loop (car (cdr step)) (car (cdr (cdr step))) (tr-add g (car step)) mono)))))
(define (hermite a d mono) (hermite-loop a d (tr-zero) mono))    ; -> (list g-tr a* d*), d* squarefree

; integrate a proper rational function a/d of a primitive monomial (certified)
;   (list 'ok g-tr 'none)        purely rational antiderivative g
;   (list 'ok g-tr (list 'log G)) rational part g plus a new log
;   (list 'partial g-tr a* d*)   reduced, but the squarefree remainder is unresolved
(define (integrate-proper a d mono)
  (let ((H (hermite a d mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (cond ((rfpoly-zero? as) (list 'ok g 'none))
            (else (let ((nl (try-newlog (list as ds) mono)))
                    (if (equal? nl 'none) (list 'partial g as ds) (list 'ok g nl))))))))

; certificate: does the result's derivative equal the integrand a/d ?
(define (proper-verify a d mono res)
  (let ((g (car (cdr res))))
    (cond ((equal? (car res) 'partial) #f)
          ((equal? (car (cdr (cdr res))) 'none) (tr-equal? (tr-deriv g mono) (list a d)))
          (else (let ((G (car (cdr (car (cdr (cdr res)))))))     ; the log argument
                  (tr-equal? (tr-add (tr-deriv g mono) (list (Drf G mono) G)) (list a d)))))))
