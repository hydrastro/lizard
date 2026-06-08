(import "cas/ntrischlog.lisp")
(define t1 (nt-monomial 1 (rat-one) 1))
(define one1 (nt-lift 1 (rat-one)))
(define se1 (list (list (quote exp) (rat-one))))
(define se2 (list (list (quote exp) (rat-one)) (list (quote exp) t1)))
(define t2 (nt-monomial 2 (nt-lift 1 (rat-one)) 1))
(define dlogx (rat-make (list 1) (list 0 1)))
(define sL (list (list (quote prim) dlogx)))
; base-field log
(display (let ((p (rat-make (list 1) (list -1 0 1)))) (ntl-verify 0 (quote ()) p (ntl-integrate 0 (quote ()) p)))) (display " ")
; depth-1 proper-fraction log
(display (ntl-verify-frac 1 se1 t1 (nt-add 1 t1 one1) (ntl-integrate-frac 1 se1 t1 (nt-add 1 t1 one1)))) (newline)
; depth-2 proper-fraction log
(display (ntl-verify-frac 2 se2 (nt-deriv 2 se2 t2) (nt-add 2 t2 (nt-lift 2 (rat-one))) (ntl-integrate-frac 2 se2 (nt-deriv 2 se2 t2) (nt-add 2 t2 (nt-lift 2 (rat-one)))))) (display " ")
; nested log
(display (ntl-verify-frac 1 sL (nt-deriv 1 sL t1) t1 (ntl-integrate-frac 1 sL (nt-deriv 1 sL t1) t1))) (newline)
; soundness: non-log-derivative fraction is not elementary
(display (ntl-elem? (ntl-integrate-frac 2 se2 (nt-lift 2 (rat-one)) (nt-add 2 t2 (nt-lift 2 (rat-one)))))) (newline)
