; -*- lisp -*-
; lib/cas/tower2int.lisp -- height-two integration: Hermite rational part plus a logarithmic term.
;
; Building on the height-two Hermite reduction (tower2herm.lisp), this module adds recognition of a
; logarithmic term, giving a genuine -- if not yet fully general -- integrator for a rational function
; of a second monomial theta2 (primitive over K1 = Q(x)(theta1), i.e. a logarithm).  After Hermite
; reduces A/D to a rational part g plus a remainder A*/D* with D* squarefree in theta2, the remainder
; is a logarithmic derivative exactly when A* = c D2(D*) for a constant c, in which case its integral
; is c log(D*).  The recognizer divides A* by D2(D*) over K1[theta2]; if the division is exact with a
; quotient that is a constant of the tower (its derivative under D2 vanishes, so it lies in Q), that
; quotient is the residue.  The full answer g + c log(D*) is certified by differentiating it with the
; two-level derivation and checking equality with A/D over K1[theta2].  This resolves the single-log
; case, where the squarefree remainder's denominator is itself the logarithm's argument; the general
; height-two Rothstein-Trager RootSum, with several constant residues found from a resultant over K1,
; is the next rung.  Builds on tower2herm.lisp.

(import "cas/tower2herm.lisp")

; is a K1 element a constant of the tower (derivative zero) ? then it is in Q
(define (k1-constant? c mono1) (tr-equal? (tr-deriv c mono1) (tr-zero)))

; recognize INT A*/D* = c log(D*) :  (list 'log c D*) | 'none
(define (h2-newlog As Ds Dth2 mono1)
  (let ((dDs (t2-deriv Ds Dth2 mono1)))
    (if (h2-zero? dDs) 'none
        (let ((dm (h2-divmod As dDs)))
          (if (h2-zero? (car (cdr dm)))
              (let ((q (car dm)))
                (if (<= (h2-deg q) 0)
                    (if (k1-constant? (if (null? q) (k1-zero) (car q)) mono1) (list 'log (if (null? q) (k1-zero) (car q)) Ds) 'none)
                    'none))
              'none)))))

; integrate a proper rational function A/D of theta2 over K1 (theta2 primitive)
;   (list 'ok g 'none)            purely rational antiderivative g
;   (list 'ok g (list 'log c v))  rational part g plus c log(v)
;   (list 'partial g A* D*)       reduced, logarithmic remainder unresolved
(define (int-h2 A D Dth2 mono1)
  (let ((H (h2-hermite A D Dth2 mono1)))
    (let ((g (car H)) (As (car (cdr H))) (Ds (car (cdr (cdr H)))))
      (if (h2-zero? As) (list 'ok g 'none)
          (let ((nl (h2-newlog As Ds Dth2 mono1)))
            (if (equal? nl 'none) (list 'partial g As Ds) (list 'ok g nl)))))))

; certificate: derivative of the returned answer equals A/D
(define (int-h2-deriv res Dth2 mono1)         ; -> height-two rational (N D) for D2(answer)
  (let ((g (car (cdr res))) (lg (car (cdr (cdr res)))))
    (let ((dg (h2tr-deriv (car g) (car (cdr g)) Dth2 mono1)))
      (if (equal? lg 'none) dg
          (h2tr-add dg (list (h2-cscale (car (cdr lg)) (t2-deriv (car (cdr (cdr lg))) Dth2 mono1)) (car (cdr (cdr lg)))))))))
(define (int-h2-verify A D Dth2 mono1)
  (let ((res (int-h2 A D Dth2 mono1)))
    (if (equal? (car res) 'ok) (h2tr-equal? (int-h2-deriv res Dth2 mono1) (list A D)) #f)))
(define (int-h2-elementary? A D Dth2 mono1) (equal? (car (int-h2 A D Dth2 mono1)) 'ok))
