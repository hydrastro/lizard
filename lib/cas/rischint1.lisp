; -*- lisp -*-
; lib/cas/rischint1.lisp -- the unified HEIGHT-1 Risch integration decider: decides INT f dx for f in a height-1
; tower K_1 = Q(x)(theta), theta = exp(b) OR log(b), by reducing to the general coupled tower-field RDE
; (rischcoupled.lisp) with a ZERO multiplier coefficient -- so INT f is exactly the y with D y = f.  This is the
; capstone over the RDE machinery: ONE entry point integrating over either kind of height-1 transcendental
; extension, dispatching on the level type and bottoming out at the rational RDE (rischrde) over Q(x)
; (docs/TRAGER_ROADMAP.md, the summit, "even more").
;
; The reduction.  Antidifferentiation INT f = y in K_1 is solving D y = f, which is the homogeneous-coefficient
; case of the coupled RDE D y + 0*y = f.  For an EXPONENTIAL level this decouples by theta-degree into base RDEs
; y_k' + k b' y_k = f_k (rc-exp-solve with the zero coefficient list); for a LOGARITHMIC level the degree-
; shifting derivation is solved top-down (rc-log-solve with f0 = 0).  A solution proves elementarity (and is
; certified by re-differentiating in K_1); its absence -- a non-terminating tail or an unsolvable base RDE --
; proves non-elementarity.  This recovers the classic verdicts through the full RDE descent: INT e^x = e^x,
; INT e^x/x = Ei non-elementary, INT log x = x log x - x, INT 1/log x ... (the log-of-x integrand 1/theta needs
; the Laurent extension; here we cover the polynomial-in-theta integrands, the core case).
;
; Public:
;   ri1-integrate-exp b g    -> (list 'elementary y) | (list 'non-elementary ...) : INT (sum g_k (exp b)^k) dx
;   ri1-integrate-log u g    -> (list 'elementary y) | (list 'non-elementary ...) : INT (sum g_k (log b)^k) dx,
;                               u = b'/b
;   ri1-certify-exp b g y    -> #t iff D y = g over Q(x)(exp b)
;   ri1-certify-log u g y    -> #t iff D y = g over Q(x)(log b)
;
; Verified: INT e^x = e^x; INT x e^x = (x-1) e^x; INT e^x/x (Ei) non-elementary; INT log x = x log x - x;
; INT x log x ... ; all through the unified coupled-RDE reduction, certified.
;
; Builds on rischcoupled.lisp (the coupled RDE) and tower.lisp / poly.lisp.

(import "cas/rischcoupled.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (ri1-len l) (if (null? l) 0 (+ 1 (ri1-len (cdr l)))))

; ----- exponential level: INT f = solve D y = f, the coupled solver with zero coefficient, bottom-up -----
(define (ri1-integrate-exp b g) (ri1-exp-result (rc-exp-solve b (list (rat-zero)) g (+ 2 (ri1-len g)))))
(define (ri1-exp-result v) (cond ((equal? (car v) (quote solvable)) (list (quote elementary) (car (cdr v)))) ((equal? (car v) (quote non-elementary)) (list (quote non-elementary) (quote tower-rde-tail))) (else (list (quote non-elementary) (quote no-rational-rde)))))
(define (ri1-certify-exp b g y) (rc-exp-certify b (list (rat-zero)) g y))

; ----- logarithmic level: INT f = solve D y = f top-down, zero coefficient -----
(define (ri1-integrate-log u g) (ri1-log-result (rc-log-solve u (rat-zero) g)))
(define (ri1-log-result v) (if (equal? (car v) (quote solvable)) (list (quote elementary) (car (cdr v))) (list (quote non-elementary) (quote no-rational-rde))))
(define (ri1-certify-log u g y) (rc-log-certify u (rat-zero) g y))
