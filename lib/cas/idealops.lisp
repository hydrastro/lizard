; -*- lisp -*-
; lib/cas/idealops.lisp — ideal operations and polynomial system solving on top of
; Groebner bases, composed with the univariate exact-root solver.
;
;   * elimination ideal: from a lex Groebner basis, the generators involving only
;     the later variables (Elimination Theorem) -- this projects a variety.
;   * ideal sum  I+J = <F union G>.
;   * ideal intersection  I ∩ J  via the classic t-trick: with a fresh variable t
;     ordered above the rest, compute a Groebner basis of {t*f} ∪ {(1-t)*g} and
;     eliminate t; what remains generates I ∩ J.
;   * zero-dimensional solving: a lex basis is triangular, so the generator in the
;     last variable alone is a univariate polynomial -- handed to solve-poly it
;     yields exact roots (rational, surd, or RootOf), connecting the multivariate
;     and univariate machinery.
;
; Every result is checkable by ideal membership (normal form = 0) via groebner.lisp.

(import "cas/groebner.lisp")
(import "cas/solve.lisp")

; ---------- elimination ----------
(define (mono-low-zero? m k) (cond ((= k 0) #t) ((null? m) #t) ((not (= (car m) 0)) #f) (else (mono-low-zero? (cdr m) (- k 1)))))
(define (poly-only-from? p k) (cond ((null? p) #t) ((mono-low-zero? (cdr (car p)) k) (poly-only-from? (cdr p) k)) (else #f)))
(define (elim-ideal G k) (filter (lambda (g) (poly-only-from? g k)) G))   ; eliminate first k variables

; ---------- ideal sum ----------
(define (ideal-sum F1 F2) (reduced-groebner (append F1 F2)))

; ---------- ideal intersection via the t-trick ----------
(define (lift-poly p) (map (lambda (t) (cons (car t) (cons 0 (cdr t)))) p))         ; prepend a t-exponent of 0
(define (times-t p v) (term-mul (cons 1 (cons 1 (zero-mono v))) p))                  ; multiply (lifted) p by t
(define (strip-t p) (map (lambda (t) (cons (car t) (cdr (cdr t)))) p))               ; drop the t-exponent
(define (ideal-intersect F1 F2 v)
  (let ((G (reduced-groebner (append (map (lambda (f) (times-t (lift-poly f) v)) F1)
                                     (map (lambda (h) (mpoly-sub (lift-poly h) (times-t (lift-poly h) v))) F2)))))
    (map strip-t (filter (lambda (p) (poly-only-from? p 1)) G))))

; ---------- zero-dimensional solving: univariate generator -> exact roots ----------
(define (irange a b) (if (> a b) '() (cons a (irange (+ a 1) b))))
(define (uni-deg p v acc) (if (null? p) acc (uni-deg (cdr p) v (max acc (gnth (cdr (car p)) (- v 1))))))
(define (uni-coeff p v e) (cond ((null? p) 0) ((= (gnth (cdr (car p)) (- v 1)) e) (car (car p))) (else (uni-coeff (cdr p) v e))))
(define (mv->uni p v) (map (lambda (e) (uni-coeff p v e)) (irange 0 (uni-deg p v 0))))  ; dense univariate (low->high)
(define (last-generator G v) (lg (filter (lambda (g) (poly-only-from? g (- v 1))) G)))
(define (lg lst) (if (null? lst) #f (car lst)))
; solve the last variable of a triangular lex basis exactly
(define (solve-last G v) (let ((g (last-generator G v))) (if (pair? g) (solve-poly (mv->uni g v)) 'none)))
(define (solve-last->string G v) (solutions->string (solve-last G v)))
