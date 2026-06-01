; -*- lisp -*-
; lib/cas/groebner.lisp — multivariate polynomials over Q and Groebner bases.
;
; Monomials are exponent vectors (e_1 ... e_v) over v variables; terms are
; (coeff . monomial) with coeff a nonzero rational; a polynomial is a list of terms
; sorted in DESCENDING lexicographic monomial order, with no zero coefficients and
; no repeated monomials.  The zero polynomial is ().
;
; Buchberger's algorithm computes a Groebner basis G of the ideal <F>: process the
; S-polynomial of each pair, reduce it to its normal form modulo the current basis,
; and add any nonzero remainder, until none are left.  Because every G is built from
; F by ideal operations, <G> = <F>; and G is certified to be a *Groebner* basis by
; Buchberger's criterion -- every S-polynomial reduces to 0 modulo G.  Normal form
; then decides ideal membership (p in <F> iff its normal form is 0), and a lex basis
; performs elimination (a generator involving only the later variables).
;
; Self-contained over Q.

(define (gnth lst i) (if (= i 0) (car lst) (gnth (cdr lst) (- i 1))))

; ---------- monomials ----------
(define (mono-mul a b) (if (null? a) '() (cons (+ (car a) (car b)) (mono-mul (cdr a) (cdr b)))))
(define (mono-div a b) (if (null? a) '() (cons (- (car a) (car b)) (mono-div (cdr a) (cdr b)))))
(define (mono-lcm a b) (if (null? a) '() (cons (max (car a) (car b)) (mono-lcm (cdr a) (cdr b)))))
(define (mono-div? m d) (cond ((null? m) #t) ((>= (car m) (car d)) (mono-div? (cdr m) (cdr d))) (else #f)))
(define (mono-lex> a b) (cond ((null? a) #f) ((> (car a) (car b)) #t) ((< (car a) (car b)) #f) (else (mono-lex> (cdr a) (cdr b)))))
(define (zero-mono v) (if (= v 0) '() (cons 0 (zero-mono (- v 1)))))

; ---------- term / polynomial accessors ----------
(define (mpoly-lt p) (car p))
(define (mpoly-lc p) (car (car p)))
(define (mpoly-lm p) (cdr (car p)))
(define (mpoly-zero? p) (null? p))

; ---------- arithmetic ----------
(define (mpoly-add p q)
  (cond ((null? p) q) ((null? q) p)
    (else (let ((tp (car p)) (tq (car q)))
      (cond ((mono-lex> (cdr tp) (cdr tq)) (cons tp (mpoly-add (cdr p) q)))
            ((mono-lex> (cdr tq) (cdr tp)) (cons tq (mpoly-add p (cdr q))))
            (else (let ((c (+ (car tp) (car tq))))
                    (if (= c 0) (mpoly-add (cdr p) (cdr q)) (cons (cons c (cdr tp)) (mpoly-add (cdr p) (cdr q)))))))))))
(define (mpoly-neg p) (map (lambda (t) (cons (- 0 (car t)) (cdr t))) p))
(define (mpoly-sub p q) (mpoly-add p (mpoly-neg q)))
; multiply polynomial by a single term (c . m); preserves descending order
(define (term-mul t p) (map (lambda (s) (cons (* (car t) (car s)) (mono-mul (cdr t) (cdr s)))) p))
(define (mpoly-mul p q) (if (null? p) '() (mpoly-add (term-mul (car p) q) (mpoly-mul (cdr p) q))))
(define (mpoly-monic p) (if (null? p) p (term-mul (cons (/ 1 (mpoly-lc p)) (zero-mono (length (mpoly-lm p)))) p)))

; ---------- multivariate division: normal form of f modulo list G ----------
(define (find-divisor m G) (cond ((null? G) #f) ((mono-div? m (mpoly-lm (car G))) (car G)) (else (find-divisor m (cdr G)))))
(define (nf-go f G rem)
  (if (null? f) (reverse rem)
    (let ((g (find-divisor (mpoly-lm f) G)))
      (if (pair? g)
          (nf-go (mpoly-sub f (term-mul (cons (/ (mpoly-lc f) (mpoly-lc g)) (mono-div (mpoly-lm f) (mpoly-lm g))) g)) G rem)
          (nf-go (cdr f) G (cons (car f) rem))))))
(define (nf f G) (nf-go f G '()))

; ---------- S-polynomial ----------
(define (spoly f g)
  (let ((l (mono-lcm (mpoly-lm f) (mpoly-lm g))))
    (mpoly-sub (term-mul (cons (/ 1 (mpoly-lc f)) (mono-div l (mpoly-lm f))) f)
              (term-mul (cons (/ 1 (mpoly-lc g)) (mono-div l (mpoly-lm g))) g))))

; ---------- Buchberger ----------
(define (all-pairs F) (if (null? F) '() (append (map (lambda (g) (cons (car F) g)) (cdr F)) (all-pairs (cdr F)))))
(define (pairs-with G r) (map (lambda (g) (cons g r)) G))
(define (bb G pairs)
  (if (null? pairs) G
    (let ((r (nf (spoly (car (car pairs)) (cdr (car pairs))) G)))
      (if (null? r) (bb G (cdr pairs))
          (bb (append G (list r)) (append (cdr pairs) (pairs-with G r)))))))
(define (groebner F) (bb F (all-pairs F)))

; minimal, reduced, monic Groebner basis (canonical up to ordering)
(define (lm-divides-another? g rest) (cond ((null? rest) #f) ((mono-div? (mpoly-lm g) (mpoly-lm (car rest))) #t) (else (lm-divides-another? g (cdr rest)))))
(define (minimalize-go done todo)
  (if (null? todo) done
    (if (or (lm-divides-another? (car todo) done) (lm-divides-another? (car todo) (cdr todo)))
        (minimalize-go done (cdr todo))
        (minimalize-go (append done (list (car todo))) (cdr todo)))))
(define (others lst i j) (cond ((null? lst) '()) ((= i j) (others (cdr lst) (+ i 1) j)) (else (cons (car lst) (others (cdr lst) (+ i 1) j)))))
(define (autoreduce-go G i)
  (if (>= i (length G)) G
    (autoreduce-go (set-at G i (nf (gnth G i) (others G 0 i))) (+ i 1))))
(define (set-at lst i v) (if (= i 0) (cons v (cdr lst)) (cons (car lst) (set-at (cdr lst) (- i 1) v))))
(define (reduced-groebner F) (map mpoly-monic (autoreduce-go (minimalize-go '() (map mpoly-monic (groebner F))) 0)))

; ---------- certificates / queries ----------
(define (in-ideal? p G) (null? (nf p G)))
(define (all-reduce? pairs G) (cond ((null? pairs) #t) ((null? (nf (spoly (car (car pairs)) (cdr (car pairs))) G)) (all-reduce? (cdr pairs) G)) (else #f)))
(define (groebner-ok? G) (all-reduce? (all-pairs G) G))           ; Buchberger's criterion
(define (generators-reduce? F G) (cond ((null? F) #t) ((null? (nf (car F) G)) (generators-reduce? (cdr F) G)) (else #f)))

; ---------- display ----------
(define (mono->str m vars) (let ((r (mstr m vars ""))) (if (equal? r "") "1" r)))
(define (mstr m vars acc)
  (cond ((null? m) acc)
        ((= (car m) 0) (mstr (cdr m) (cdr vars) acc))
        (else (let ((piece (if (= (car m) 1) (car vars) (string-append (car vars) "^" (number->string (car m))))))
                (mstr (cdr m) (cdr vars) (if (equal? acc "") piece (string-append acc "*" piece)))))))
(define (qn c) (if (integer? c) (number->string c) (string-append (number->string (numerator c)) "/" (number->string (denominator c)))))
(define (term->str t vars)
  (let ((mon (mono->str (cdr t) vars)) (c (car t)))
    (cond ((equal? mon "1") (qn c)) ((= c 1) mon) ((= c -1) (string-append "-" mon)) (else (string-append (qn c) "*" mon)))))
(define (term->abs t vars) (term->str (cons (abs (car t)) (cdr t)) vars))
(define (mpoly->str p vars) (if (null? p) "0" (pstr (cdr p) vars (term->str (car p) vars))))
(define (pstr p vars acc)
  (if (null? p) acc
    (if (< (car (car p)) 0)
        (pstr (cdr p) vars (string-append acc " - " (term->abs (car p) vars)))
        (pstr (cdr p) vars (string-append acc " + " (term->str (car p) vars))))))
