; -*- lisp -*-
; lib/cas/sturm.lisp — exact real-root counting and isolation via Sturm sequences.
;
; For a univariate polynomial over Q, the canonical Sturm chain is p_0 = p, p_1 = p',
; p_{k+1} = -rem(p_{k-1}, p_k).  By Sturm's theorem the number of DISTINCT real roots
; in an interval (a, b] equals V(a) - V(b), where V(x) is the number of sign changes in
; the chain evaluated at x (zeros skipped).  We make p squarefree first (divide by
; gcd(p, p')) so every root is simple; then the chain counts exactly the distinct real
; roots, multiplicities aside.
;
; All real roots lie within the Cauchy bound M = 1 + max|a_i|/|a_n|, so the total real
; root count is the count on (-M, M].  Isolation bisects [-M, M], using the count to
; split, until every returned rational interval holds exactly one root; because p is
; squarefree each isolating interval shows a strict sign change p(lo)*p(hi) < 0, which
; serves as an independent certificate.  Everything is exact rational arithmetic.
;
; Builds on poly.lisp.

(import "cas/poly.lisp")

(define (labs x) (if (< x 0) (- 0 x) x))
(define (lead-coeff p) (poly-coeff p (poly-deg p)))
(define (sqfree-part p) (car (poly-divmod p (poly-gcd p (poly-deriv p)))))

; ---------- Sturm chain ----------
(define (sc-build a b acc)
  (if (poly-zero? b) acc
    (let ((r (poly-neg (car (cdr (poly-divmod a b))))))
      (if (poly-zero? r) acc (sc-build b r (append acc (list r)))))))
(define (sturm-chain p) (if (poly-zero? (poly-deriv p)) (list p) (sc-build p (poly-deriv p) (list p (poly-deriv p)))))

; ---------- sign variations ----------
(define (sign-of v) (cond ((< v 0) -1) ((> v 0) 1) (else 0)))
(define (sign-at poly x) (sign-of (poly-eval poly x)))
(define (vc signs last cnt)
  (cond ((null? signs) cnt)
        ((= (car signs) 0) (vc (cdr signs) last cnt))
        ((= last 0) (vc (cdr signs) (car signs) cnt))
        ((= (car signs) last) (vc (cdr signs) last cnt))
        (else (vc (cdr signs) (car signs) (+ cnt 1)))))
(define (vchanges chain x) (vc (map (lambda (poly) (sign-at poly x)) chain) 0 0))
(define (count-sf chain a b) (- (vchanges chain a) (vchanges chain b)))

; ---------- Cauchy root bound ----------
(define (maxc p i n acc) (if (>= i n) acc (maxc p (+ i 1) n (max acc (labs (poly-coeff p i))))))
(define (cauchy-bound p) (+ 1 (/ (maxc p 0 (poly-deg p) 0) (labs (lead-coeff p)))))

; ---------- public counting ----------
; distinct real roots of p in (a, b]
(define (count-real-roots p a b) (count-sf (sturm-chain (sqfree-part p)) a b))
; total number of distinct real roots
(define (num-real-roots p)
  (if (< (poly-deg p) 1) 0
    (let ((sf (sqfree-part p))) (let ((m (cauchy-bound sf))) (count-sf (sturm-chain sf) (- 0 m) m)))))

; ---------- isolation ----------
; choose a midpoint of (lo,hi) that is not itself a root (only finitely many roots exist,
; so halving toward the non-root lo endpoint terminates); this keeps every interval
; endpoint a non-root, guaranteeing a strict sign change across each isolating interval
(define (nrm sf lo hi m) (if (= (poly-eval sf m) 0) (nrm sf lo hi (/ (+ lo m) 2)) m))
(define (nonroot-mid sf lo hi) (nrm sf lo hi (/ (+ lo hi) 2)))
(define (iso sf chain lo hi)
  (let ((c (count-sf chain lo hi)))
    (cond ((= c 0) '())
          ((= c 1) (list (list lo hi)))
          (else (let ((mid (nonroot-mid sf lo hi))) (append (iso sf chain lo mid) (iso sf chain mid hi)))))))
(define (isolate-roots p)
  (if (< (poly-deg p) 1) '()
    (let ((sf (sqfree-part p))) (let ((m (cauchy-bound sf))) (iso sf (sturm-chain sf) (- 0 m) m)))))

; refine an isolating interval (lo hi) of squarefree sf to width < eps by bisection
(define (refine-iv sf lo hi eps)
  (if (< (- hi lo) eps) (list lo hi)
    (let ((mid (/ (+ lo hi) 2)))
      (if (= (sign-at sf mid) 0) (list mid mid)
          (if (= (sign-at sf lo) (sign-at sf mid)) (refine-iv sf mid hi eps) (refine-iv sf lo mid eps))))))
(define (isolate-refined p eps)
  (let ((sf (sqfree-part p))) (map (lambda (iv) (refine-iv sf (car iv) (car (cdr iv)) eps)) (isolate-roots p))))

; ---------- certificates ----------
; each isolating interval contains exactly one root and shows a strict sign change
(define (iv-ok? p iv) (and (= (count-real-roots p (car iv) (car (cdr iv))) 1) (< (* (poly-eval (sqfree-part p) (car iv)) (poly-eval (sqfree-part p) (car (cdr iv)))) 0)))
(define (isolation-ok? p ivs) (cond ((null? ivs) #t) ((iv-ok? p (car ivs)) (isolation-ok? p (cdr ivs))) (else #f)))

; ---------- display ----------
(define (q->s x) (if (integer? x) (number->string x) (string-append (number->string (numerator x)) "/" (number->string (denominator x)))))
(define (iv->string iv) (string-append "(" (q->s (car iv)) ", " (q->s (car (cdr iv))) ")"))
(define (intervals->string ivs) (if (null? ivs) "no real roots" (iv-go ivs "")))
(define (iv-go ivs acc) (if (null? ivs) acc (iv-go (cdr ivs) (if (equal? acc "") (iv->string (car ivs)) (string-append acc ", " (iv->string (car ivs)))))))
