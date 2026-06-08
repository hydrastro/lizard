; -*- lisp -*-
; lib/cas/rt-tower.lisp — close the deferred algebraic-residue cases of integration
; over a primitive monomial (theta = log x) by routing the substituted integrand
; through the Rothstein-Trager logarithmic part (rt.lisp), which handles residues
; that are algebraic numbers.
;
; For INT (1/x) R(log x) dx with R = num/den, the substitution t = log x gives the
; ordinary rational integral INT R(t) dt.  The existing reducer (elem.lisp ->
; integrate.lisp) solves this whenever the logarithmic part has rational residues
; (and does the polynomial part, Hermite rational part, and arctangents); it only
; declines when the residues are genuinely algebraic, e.g. INT 1/(t^2-2) dt whose
; antiderivative needs sqrt(2).  In that case, when den is squarefree, we split off
; the polynomial part by division and hand the proper part R/den to rt-log-part,
; whose answer is a RootSum over the algebraic residues.  Rothstein-Trager's own
; certificate (rt-certificate: a fully rational identity p = sum_c c g_c' (q/g_c))
; verifies the logarithmic part exactly, and the chain rule ((1/x)dx = d log x)
; lifts it back to x.  So an answer is returned only when it is certified.
;
; Top-level helpers only; builds on elem.lisp and rt.lisp.

(import "cas/elem.lisp")
(import "cas/rt.lisp")

(define (squarefree-poly? q) (= (poly-deg (poly-gcd q (poly-deriv q))) 0))
(define (pad-int p k) (if (null? p) '() (cons (/ (car p) k) (pad-int (cdr p) (+ k 1)))))
(define (poly-antideriv p) (cons 0 (pad-int p 1)))             ; INT (poly in t) dt, zero constant
(define (tower-combine polyanti terms)
  (let ((ps (if (poly-zero? polyanti) "" (poly->string polyanti "log(x)")))
        (ls (rt-log->string-v terms "log(x)")))
    (cond ((equal? ps "") (string-append ls " + C"))
          ((equal? ls "0") (string-append ps " + C"))
          (else (string-append ps " + " ls " + C")))))

; INT (1/x) R(log x) dx, R = num/den, closing algebraic-residue cases
(define (int-tower-log-rt num den)
  (let ((qr (poly-divmod num den)))                            ; num = Q*den + R
    (let ((terms (rt-log-part (car (cdr qr)) den)))
      (if (rt-certificate (car (cdr qr)) den terms)
          (list 'ok (tower-combine (poly-antideriv (car qr)) terms) #t)
          (list 'cannot 'rt-certificate-failed)))))
(define (int-tower-log num den)
  (let ((basic (integrate-primitive-log num den)))             ; try the rational/Hermite/arctan reducer first
    (if (equal? (car basic) 'ok) basic
        (if (squarefree-poly? den) (int-tower-log-rt num den) (list 'cannot 'needs-hermite)))))
(define (tower-result->string r) (if (equal? (car r) 'ok) (car (cdr r)) "not resolved"))
(define (tower-certified? r) (and (equal? (car r) 'ok) (car (cdr (cdr r)))))
