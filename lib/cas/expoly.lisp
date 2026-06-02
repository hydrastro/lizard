; -*- lisp -*-
; lib/cas/expoly.lisp -- integration of the exponential polynomial part of a height-one tower:
; a Laurent polynomial  sum_k a_k(x) theta^k  in theta = e^p (p a polynomial, a_k rational in x).
;
; This is the part the tower Hermite reduction cannot reach.  Because D(b theta^k) = (b' + k p' b)
; theta^k, integrating a_k theta^k means solving the Risch differential equation b' + (k p') b = a_k
; over Q(x) -- exactly INT a_k e^{k p} -- handled by rischde.lisp/int-rat-exp.  The terms e^{k p}
; for distinct k are linearly independent, so by Liouville's theorem the whole sum is elementary iff
; every term is; we integrate term by term and report "non-elementary" the moment one term fails
; (which is how INT e^x/x and INT e^{x^2} are proved impossible).  The k = 0 term is an ordinary
; base-field integral, done by ratfull.lisp.  Every coefficient is checked by re-deriving its
; defining equation, b' + k p' b = a_k, and the k = 0 part by differentiation.
; Builds on rderat.lisp (hence rischde.lisp) and ratfull.lisp.

(import "cas/rderat.lisp")
(import "cas/ratfull.lisp")

(define (ep-k tm) (car tm))          ; a term is (k anum aden): coefficient a_k = anum/aden on theta^k
(define (ep-an tm) (car (cdr tm)))
(define (ep-ad tm) (car (cdr (cdr tm))))

; INT a_k e^{k p} = b_k e^{k p}  ->  (k bnum bden) | 'none
(define (ep-term k anum aden p)
  (let ((r (int-rat-exp (cons anum aden) (poly-scale k p))))
    (if (equal? (car r) 'non-elementary) 'none
        (list k (car (car (cdr r))) (cdr (car (cdr r)))))))

(define (ep-loop terms p eacc base)
  (cond ((null? terms) (list 'elementary (reverse eacc) base))
        ((= (ep-k (car terms)) 0)
         (ep-loop (cdr terms) p eacc (rat-integrate-full (ep-an (car terms)) (ep-ad (car terms)))))
        (else (let ((t (ep-term (ep-k (car terms)) (ep-an (car terms)) (ep-ad (car terms)) p)))
                (if (equal? t 'none) (list 'non-elementary)
                    (ep-loop (cdr terms) p (cons t eacc) base))))))

; INT (sum_k a_k theta^k) dx  ->  (list 'elementary expterms basepart) | (list 'non-elementary)
;   expterms : list of (k bnum bden), the coefficient b_k of theta^k in the answer
;   basepart : (polypart ratnum ratden logterms complete?) from ratfull, the integral of the k=0 term
(define (int-exp-poly terms p) (ep-loop terms p '() (list '() '() (list 1) '() #t)))

; --- certificate: re-derive each coefficient's defining equation ---
(define (ep-verify-term k bnum bden anum aden p)
  (let ((b (rde-rmake bnum bden)))
    (let ((lhs (rde-radd (rde-rderiv b) (rde-rmul (rde-rmake (poly-scale k (poly-deriv p)) (list 1)) b))))
      (rde-rzero? (rde-rsub lhs (rde-rmake anum aden))))))
(define (ep-find k expterms)
  (cond ((null? expterms) #f) ((= (car (car expterms)) k) (car expterms)) (else (ep-find k (cdr expterms)))))
(define (ep-check-terms terms expterms p)
  (cond ((null? terms) #t)
        ((= (ep-k (car terms)) 0) (ep-check-terms (cdr terms) expterms p))
        (else (let ((bt (ep-find (ep-k (car terms)) expterms)))
                (if (if bt (ep-verify-term (car bt) (car (cdr bt)) (car (cdr (cdr bt))) (ep-an (car terms)) (ep-ad (car terms)) p) #f)
                    (ep-check-terms (cdr terms) expterms p) #f)))))
(define (ep-base-ok terms)
  (cond ((null? terms) #t)
        ((= (ep-k (car terms)) 0) (rat-integrate-full-verify (ep-an (car terms)) (ep-ad (car terms))))
        (else (ep-base-ok (cdr terms)))))
(define (int-exp-poly-verify terms p)
  (let ((r (int-exp-poly terms p)))
    (if (equal? (car r) 'non-elementary) #f
        (if (ep-check-terms terms (car (cdr r)) p) (ep-base-ok terms) #f))))
(define (int-exp-poly-elementary? terms p) (equal? (car (int-exp-poly terms p)) 'elementary))
