; -*- lisp -*-
; lib/cas/ratfull.lisp -- complete rational-function integration for an ARBITRARY A/D, including
; improper fractions (deg A >= deg D).
;
; rischrat.lisp's rat-integrate assumes a proper fraction; on an improper one Hermite reduction
; silently drops the polynomial part.  The fix is the standard one: divide A = Q D + Rem, so that
;   INT A/D = INT Q  +  INT Rem/D,
; where INT Q is the elementary polynomial antiderivative and Rem/D is proper and handled by
; rat-integrate (Hermite + Rothstein-Trager).  The answer is returned as
;   (polypart ratnum ratden logterms complete?)
; and certified by differentiating the whole thing back to A/D when the residues are rational;
; the polynomial division A = Q D + Rem and the Hermite rational part are exact regardless.
; Builds on rischrat.lisp.

(import "cas/rischrat.lisp")

(define (rif-pint p k) (if (null? p) '() (cons (/ (car p) (+ k 1)) (rif-pint (cdr p) (+ k 1)))))
(define (rif-antideriv p) (poly-norm (cons 0 (rif-pint p 0))))    ; polynomial antiderivative, zero constant
(define (rif-cadr l) (car (cdr l)))
(define (rif-caddr l) (car (cdr (cdr l))))
(define (rif-cadddr l) (car (cdr (cdr (cdr l)))))

; INT A/D for arbitrary A/D -> (polypart ratnum ratden logterms complete?)
(define (rat-integrate-full A D)
  (if (poly-const? D)
      (list (rif-antideriv (poly-scale (/ 1 (poly-coeff D 0)) A)) '() (list 1) '() #t)
      (let ((qr (poly-divmod A D)))
        (if (poly-zero? (rif-cadr qr))
            (list (rif-antideriv (car qr)) '() (list 1) '() #t)
            (cons (rif-antideriv (car qr)) (rat-integrate (rif-cadr qr) D))))))

(define (rif-polypart r) (car r))
(define (rif-complete? r) (rif-cadddr (cdr r)))

; certificate: d/dx(polypart + ratnum/ratden + sum c log v) = A/D  (when residues rational)
(define (rat-integrate-full-verify A D)
  (let ((res (rat-integrate-full A D)))
    (let ((pp (car res)) (rest (cdr res)))
      (let ((rn (car rest)) (rd (rif-cadr rest)) (terms (rif-caddr rest)) (cmpl (rif-cadddr rest)))
        (if cmpl
            (let ((V (ros-prod (ros-args terms))))
              (let ((s1 (hm-radd (poly-deriv pp) (list 1) (car (hm-ratderiv rn rd)) (cdr (hm-ratderiv rn rd)))))
                (let ((s2 (hm-radd (car s1) (cdr s1) (ros-deriv-numer terms V) V)))
                  (poly-zero? (poly-sub (poly-mul (car s2) D) (poly-mul A (cdr s2)))))))
            #f)))))
; always-true part: polynomial division + Hermite rational part are exact even when not complete
(define (rat-integrate-full-rational-ok? A D)
  (let ((qr (poly-divmod A D))) (rat-integrate-rational-ok? (rif-cadr qr) D)))
