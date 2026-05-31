; -*- lisp -*-
; lib/cas/integrate.lisp — integration of rational functions, certified by
; differentiating the answer back to the integrand.
;
; Method: partial-fraction the integrand (which factors the denominator), then
; integrate each term in closed form:
;   * polynomial part            -> polynomial (power rule)
;   * A/(a x + b)^j  (linear)     -> log (j=1) or a rational term (j>1)
;   * (Bx+C)/f^j  (irred. quad.)  -> split off the f'-proportional part
;       (gives log / rational), leaving mu/f which, for a complex-root factor
;       (disc < 0) at multiplicity 1, is an arctan.
;
; The answer is (ok ratpart logs arctans):
;   ratpart : a rational function (the algebraic part of the integral)
;   logs    : list of (coeff arg)  meaning  coeff * log(arg)
;   arctans : list of (mu f)       meaning the arctan whose derivative is mu/f,
;             i.e.  (2 mu / sqrt(D)) * arctan((2a x + b)/sqrt(D)),  D = 4ac - b^2
;
; THE CERTIFICATE: `integrate-verify` differentiates the answer and checks it
; equals the integrand EXACTLY as rational functions over Q.  Crucially the
; arctan term's derivative is the rational function mu/f (the sqrt(D) cancels by
; the identity d/dx arctan((2ax+b)/sqrt(D)) = sqrt(D)/(2f)), so the entire check
; lives in Q -- a wrong antiderivative cannot pass.
;
; Cases that need algebraic-number machinery beyond Q -- quadratic factors with
; real irrational roots (disc > 0), quadratic factors at multiplicity >= 2 with
; a nonzero arctan part, and irreducible factors of degree >= 3 -- are reported
; honestly as 'cannot rather than integrated wrongly or partially.

(import "cas/ratfun.lisp")

; ============================================================
;  antiderivative of a polynomial (power rule)
; ============================================================
(define (integ-terms p i) (if (null? p) '() (cons (/ (car p) (+ i 1)) (integ-terms (cdr p) (+ i 1)))))
(define (poly-integral p) (poly-norm (cons 0 (integ-terms p 0))))

; ============================================================
;  accumulator:  (list ratpart logs arctans)
; ============================================================
(define (acc-ratpart acc) (car acc))
(define (acc-logs acc) (car (cdr acc)))
(define (acc-arctans acc) (car (cdr (cdr acc))))
(define (add-rat acc r) (list (rat-add (acc-ratpart acc) r) (acc-logs acc) (acc-arctans acc)))
(define (add-log acc coeff arg) (list (acc-ratpart acc) (cons (list coeff arg) (acc-logs acc)) (acc-arctans acc)))
(define (add-arctan acc mu f) (list (acc-ratpart acc) (acc-logs acc) (cons (list mu f) (acc-arctans acc))))

; ============================================================
;  per-term integration
; ============================================================
(define (int-linear A f j acc)
  (let ((a (poly-lead f)) (Av (poly-coeff A 0)))
    (if (= j 1)
        (add-log acc (/ Av a) f)
        (add-rat acc (rat-make (list (/ Av (* a (- 1 j)))) (poly-pow f (- j 1)))))))

(define (int-quadratic A f j acc)
  (let ((a (poly-coeff f 2)) (b (poly-coeff f 1)) (c (poly-coeff f 0))
        (B (poly-coeff A 1)) (C (poly-coeff A 0)))
    (let ((lam (/ B (* 2 a))) (disc (- (* b b) (* 4 a c))))
      (let ((mu (- C (/ (* B b) (* 2 a)))))
        (let ((acc1 (cond ((= lam 0) acc)
                          ((= j 1) (add-log acc lam f))
                          (else (add-rat acc (rat-make (list (/ lam (- 1 j))) (poly-pow f (- j 1))))))))
          (cond ((= mu 0) acc1)
                ((and (= j 1) (negative? disc)) (add-arctan acc1 mu f))
                (else 'cannot)))))))                ; disc>0 or j>=2: beyond Q

(define (integrate-one t acc)
  (if (equal? acc 'cannot) 'cannot
    (let ((A (car t)) (f (car (cdr t))) (j (car (cdr (cdr t)))))
      (cond ((= (poly-deg f) 1) (int-linear A f j acc))
            ((= (poly-deg f) 2) (int-quadratic A f j acc))
            (else 'cannot)))))                       ; irreducible factor of degree >= 3

(define (integrate-terms terms acc)
  (if (equal? acc 'cannot) 'cannot
    (if (null? terms) acc (integrate-terms (cdr terms) (integrate-one (car terms) acc)))))

(define (integrate-rational num den)
  (let ((pf (partial-fractions num den)))
    (let ((res (integrate-terms (car (cdr pf))
                                (list (rat-make (poly-integral (car pf)) (list 1)) '() '()))))
      (if (equal? res 'cannot) (list 'cannot 'needs-algebraic-extension) (cons 'ok res)))))

; ============================================================
;  THE CERTIFICATE: differentiate the answer, compare to integrand over Q
; ============================================================
(define (rat-deriv r)
  (let ((n (rat-num r)) (d (rat-den r)))
    (rat-make (poly-sub (poly-mul (poly-deriv n) d) (poly-mul n (poly-deriv d)))
              (poly-mul d d))))
(define (log-deriv lt) (rat-make (poly-scale (car lt) (poly-deriv (car (cdr lt)))) (car (cdr lt))))
(define (arctan-deriv at) (rat-make (list (car at)) (car (cdr at))))

(define (integral-deriv ans)
  (rat-add-many (rat-deriv (acc-ratpart (cdr ans)))
                (append (map log-deriv (acc-logs (cdr ans)))
                        (map arctan-deriv (acc-arctans (cdr ans))))))

(define (integrate-verify num den ans)
  (and (equal? (car ans) 'ok) (rat-equal? (integral-deriv ans) (rat-make num den))))

; ============================================================
;  display
; ============================================================
(define (int-sqrt n)                       ; integer sqrt or #f
  (if (< n 0) #f (int-sqrt-loop n 0)))
(define (int-sqrt-loop n k) (cond ((> (* k k) n) #f) ((= (* k k) n) k) (else (int-sqrt-loop n (+ k 1)))))

(define (sqrt-rat-exact D)                 ; rational sqrt of D, or #f
  (let ((sn (int-sqrt (numerator D))) (sd (int-sqrt (denominator D))))
    (if (and sn sd) (/ sn sd) #f)))

(define (coeff-times c)                     ; "c * " unless c is +-1
  (cond ((= c 1) "") ((= c -1) "-") (else (string-append (rat->string c) " * "))))

(define (arctan-string at var)             ; (mu f) -> closed form, simplified
  (let ((mu (car at)) (f (car (cdr at))))
    (let ((a (poly-coeff f 2)) (b (poly-coeff f 1)) (c (poly-coeff f 0)))
      (let ((D (- (* 4 a c) (* b b))))
        (let ((r (sqrt-rat-exact D)))
          (if r
              (string-append (coeff-times (/ (* 2 mu) r))
                             "arctan(" (poly->string (poly-scale (/ 1 r) (list b (* 2 a))) var) ")")
              (string-append (rat->string (* 2 mu)) "/sqrt(" (rat->string D) ")"
                             " * arctan((" (poly->string (list b (* 2 a)) var)
                             ")/sqrt(" (rat->string D) "))")))))))

(define (log-string lt var)
  (string-append (coeff-times (car lt)) "log(" (poly->string (car (cdr lt)) var) ")"))

(define (join-plus parts) (if (null? parts) "" (if (null? (cdr parts)) (car parts)
  (string-append (car parts) " + " (join-plus (cdr parts))))))

(define (ratpart-string r var)
  (cond ((poly-zero? (rat-num r)) "")
        ((equal? (rat-den r) (list 1)) (poly->string (rat-num r) var))
        (else (string-append "(" (poly->string (rat-num r) var) ")/("
                             (poly->string (rat-den r) var) ")"))))

(define (nonempty xs) (if (null? xs) '() (if (equal? (car xs) "") (nonempty (cdr xs)) (cons (car xs) (nonempty (cdr xs))))))

(define (integral->string ans var)
  (if (equal? (car ans) 'cannot) "<no elementary form over Q by current method>"
    (let ((parts (nonempty (cons (ratpart-string (acc-ratpart (cdr ans)) var)
                                 (append (map (lambda (l) (log-string l var)) (acc-logs (cdr ans)))
                                         (map (lambda (a) (arctan-string a var)) (acc-arctans (cdr ans))))))))
      (if (null? parts) "0" (string-append (join-plus parts) " + C")))))
