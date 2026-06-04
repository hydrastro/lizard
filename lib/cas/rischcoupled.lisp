; -*- lisp -*-
; lib/cas/rischcoupled.lisp -- the GENERAL coupled tower-field Risch differential equation, for BOTH exponential
; and logarithmic levels of a height-1 tower K_1 = Q(x)(theta), with an arbitrary tower-element coefficient.
; This generalizes the exp-over-exp special case (rischtfrde2.lisp) to arbitrary coefficients and to the
; logarithmic level (whose derivation shifts theta-degree rather than preserving it), running the recursive
; Risch descent on the differential-equation machinery underneath at full generality for height 1
; (docs/TRAGER_ROADMAP.md, the summit, "even more").
;
; EXPONENTIAL level (theta = exp(b), theta' = b' theta).  The derivation is diagonal:
;     D(sum_k y_k theta^k) = sum_k (y_k' + k b' y_k) theta^k .
; With an arbitrary coefficient f = sum_j f_j theta^j, the equation D y + f y = g reads, at theta-degree n,
;     y_n' + (n b' + f_0) y_n + sum_{j>=1} f_j y_{n-j} = g_n ,
; a BANDED system coupling each degree to LOWER degrees -- solved BOTTOM-UP (degree 0 first): once y_0..y_{n-1}
; are known, degree n is the base-field RDE y_n' + (n b' + f_0) y_n = g_n - sum_{j>=1} f_j y_{n-j}, solved by
; rischrde one level down.  The forced higher-degree tail is watched for non-termination (the non-elementarity
; obstruction).
;
; LOGARITHMIC level (theta = log(b), theta' = u = b'/b in the base field).  The derivation SHIFTS degree down:
;     D(sum_k y_k theta^k) = sum_k (y_k' + (k+1) u y_{k+1}) theta^k ,
; so with a base-field coefficient f_0 the equation at degree k is
;     y_k' + f_0 y_k + (k+1) u y_{k+1} = g_k ,
; coupling each degree to the NEXT-HIGHER -- solved TOP-DOWN (highest degree first): the top degree m is the base
; RDE y_m' + f_0 y_m = g_m, then descend, each step a base RDE with the already-found y_{k+1} on the right.
;
; The certificate (the appropriate derivation D y + f y = g, compared coefficient-wise) is the final arbiter.
;
; Public:
;   rc-exp-solve b f g N    -> (list 'solvable y) | (list 'non-elementary 'tail) | (list 'no-rational ...) :
;       solve D y + f y = g over Q(x)(exp b), f a K_1 element (coeff list), bottom-up to bound N
;   rc-log-solve u f0 g     -> (list 'solvable y) | (list 'no-rational ...) :
;       solve D y + f0 y = g over Q(x)(log b) with u = b'/b, f0 a base coefficient, top-down
;   rc-exp-deriv b y        -> D y for the exponential level (diagonal)
;   rc-log-deriv u y        -> D y for the logarithmic level (degree-shifting)
;   rc-exp-certify b f g y  -> #t iff D y + f y = g (exponential)
;   rc-log-certify u f0 g y -> #t iff D y + f0 y = g (logarithmic)
;   rc-int-log-x            -> the certified INT log x dx = x log x - x via the log-level top-down solve
;
; Verified: exp level y'+(1+s)y=1 has a non-terminating tail (y_0=1, y_1=-1/2, y_2=1/6, ...); log level
; INT log x = x log x - x via top-down (y_1=x, y_0=-x); plus certificates and a solvable exp control.
;
; Builds on rischrde.lisp (the base RDE) and tower.lisp / poly.lisp.

(import "cas/rischrde.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (rc-nth l k) (if (= k 0) (car l) (rc-nth (cdr l) (- k 1))))
(define (rc-len l) (if (null? l) 0 (+ 1 (rc-len (cdr l)))))
(define (rc-coeff g k) (if (if (< k 0) #t (>= k (rc-len g))) (rat-zero) (rc-nth g k)))
(define (rc-reverse l) (rc-rev l (quote ())))
(define (rc-rev l acc) (if (null? l) acc (rc-rev (cdr l) (cons (car l) acc))))

; ===== EXPONENTIAL LEVEL =====
; diagonal derivation
(define (rc-exp-deriv b y) (rc-ed-go b y 0))
(define (rc-ed-go b y k) (if (null? y) (quote ()) (cons (rat-add (rat-deriv (car y)) (rat-mul (rat-scale k (rat-deriv b)) (car y))) (rc-ed-go b (cdr y) (+ k 1)))))

; the convolution sum_{j>=1} f_j y_{n-j} for known y_0..y_{n-1}
(define (rc-conv f ys n) (rc-conv-go f ys n 1))
(define (rc-conv-go f ys n j) (if (> j n) (rat-zero) (rat-add (rat-mul (rc-coeff f j) (rc-yget ys (- n j))) (rc-conv-go f ys n (+ j 1)))))
; ys is the bottom-up accumulated list (index 0 first); get index i
(define (rc-yget ys i) (if (if (< i 0) #t (>= i (rc-len ys))) (rat-zero) (rc-nth ys i)))

; solve bottom-up: degree n RDE  y_n' + (n b' + f_0) y_n = g_n - conv
(define (rc-exp-solve b f g N) (rc-es-go b f g N 0 (quote ())))
(define (rc-es-go b f g N n ys)
  (if (> n N) (rc-es-verdict g ys)
      (rc-es-step b f g N n ys (rde-solve (rat-add (rat-scale n (rat-deriv b)) (rc-coeff f 0)) (rat-sub (rc-coeff g n) (rc-conv f ys n))))))
(define (rc-es-step b f g N n ys yn)
  (if (equal? yn (quote no-rational-solution)) (list (quote no-rational) (quote rde-obstruction))
      (rc-es-go b f g N (+ n 1) (rc-append ys yn))))
(define (rc-append l v) (if (null? l) (list v) (cons (car l) (rc-append (cdr l) v))))
; verdict: tail beyond g's support nonzero -> non-terminating (non-elementary); else solvable (trimmed)
(define (rc-es-verdict g ys) (if (rc-tail-nonzero? g ys) (list (quote non-elementary) (quote tail)) (list (quote solvable) (rc-trim ys))))
(define (rc-tail-nonzero? g ys) (rc-tn ys (rc-len g) 0))
(define (rc-tn ys supp k) (cond ((null? ys) #f) ((if (>= k supp) (not (rat-zero? (car ys))) #f) #t) (else (rc-tn (cdr ys) supp (+ k 1)))))
(define (rc-trim ys) (rc-reverse (rc-dropz (rc-reverse ys))))
(define (rc-dropz l) (cond ((null? l) (quote ())) ((rat-zero? (car l)) (rc-dropz (cdr l))) (else l)))

(define (rc-exp-certify b f g y) (rc-eq? (rc-add (rc-exp-deriv b y) (rc-mul f y)) g))
; K_1 polynomial multiply (coeff lists)
(define (rc-mul a b) (if (null? a) (quote ()) (rc-add (rc-scaleL (car a) b) (cons (rat-zero) (rc-mul (cdr a) b)))))
(define (rc-scaleL c b) (if (null? b) (quote ()) (cons (rat-mul c (car b)) (rc-scaleL c (cdr b)))))
(define (rc-add a b) (cond ((null? a) b) ((null? b) a) (else (cons (rat-add (car a) (car b)) (rc-add (cdr a) (cdr b))))))
(define (rc-eq? a b) (rc-eqg a b 0 (rc-maxlen a b)))
(define (rc-maxlen a b) (if (> (rc-len a) (rc-len b)) (rc-len a) (rc-len b)))
(define (rc-eqg a b k m) (if (>= k m) #t (if (rat-equal? (rc-coeff a k) (rc-coeff b k)) (rc-eqg a b (+ k 1) m) #f)))

; ===== LOGARITHMIC LEVEL =====
; degree-shifting derivation D(sum y_k theta^k) = sum (y_k' + (k+1) u y_{k+1}) theta^k
(define (rc-log-deriv u y) (rc-ld-go u y 0 (rc-len y)))
(define (rc-ld-go u y k m) (if (>= k m) (quote ()) (cons (rat-add (rat-deriv (rc-coeff y k)) (rat-mul (rat-scale (+ k 1) u) (rc-coeff y (+ k 1)))) (rc-ld-go u y (+ k 1) m))))

; solve top-down: degree m (top) is y_m' + f_0 y_m = g_m; then descend, degree k: y_k' + f_0 y_k = g_k - (k+1) u y_{k+1}
(define (rc-log-solve u f0 g) (rc-ls-top u f0 g (- (rc-len g) 1)))
(define (rc-ls-top u f0 g m) (rc-ls-go u f0 g m (rc-zerolist (+ m 1))))
(define (rc-zerolist n) (if (= n 0) (quote ()) (cons (rat-zero) (rc-zerolist (- n 1)))))
; descend from top index m down to 0, filling ys (a fixed-length list, index 0..m)
(define (rc-ls-go u f0 g k ys)
  (if (< k 0) (rc-ls-verdict ys)
      (rc-ls-step u f0 g k ys (rde-solve f0 (rat-sub (rc-coeff g k) (rat-mul (rat-scale (+ k 1) u) (rc-yget ys (+ k 1))))))))
(define (rc-ls-step u f0 g k ys yk)
  (if (equal? yk (quote no-rational-solution)) (list (quote no-rational) (quote rde-obstruction))
      (rc-ls-go u f0 g (- k 1) (rc-set ys k yk))))
(define (rc-set ys k v) (rc-set-go ys k v 0))
(define (rc-set-go ys k v i) (if (null? ys) (quote ()) (cons (if (= i k) v (car ys)) (rc-set-go (cdr ys) k v (+ i 1)))))
(define (rc-ls-verdict ys) (list (quote solvable) (rc-trim ys)))

(define (rc-log-certify u f0 g y) (rc-eq? (rc-add (rc-log-deriv u y) (rc-scaleL f0 y)) g))

; ----- the headline: INT log x dx = x log x - x via the log-level top-down solve.
; theta = log x, u = 1/x, f0 = 0, integrand g = theta (degree 1: g = (0, 1)). -----
(define (rc-int-log-x) (rc-log-solve (rat-make (list 1) (list 0 1)) (rat-zero) (list (rat-zero) (rat-one))))
