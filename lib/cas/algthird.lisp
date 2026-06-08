; -*- lisp -*-
; lib/cas/algthird.lisp -- RUNG 3a of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): the THIRD-KIND
; ALGEBRAIC LOGARITHM for the genus-0 radical (p quadratic).
;
; A simple pole of the rational part of a differential on y^2 = p carries a nonzero residue (Rung 1): such an
; integral is THIRD KIND and its antiderivative is an algebraic logarithm c log(g) with g in K = Q(x)[y]/(y^2-p).
; For genus 0 (p a squarefree quadratic) the residue divisor is always principal, so the logarithm always
; exists -- but the previous session's probe showed the NAIVE closed form (y - sqrt(p(s)))/(x - s) is WRONG off
; the pole-at-origin case: it introduces a spurious rational-logarithm component.  The correct argument uses the
; TANGENT LINE to the curve at the point over the pole:
;
;     INT dx/((x - s) sqrt(p))  =  c * log( (y - L(x)) / (x - s) ),
;     L(x) = rho + k (x - s),   rho^2 = p(s),   k = p'(s) / (2 rho)   (the curve's slope at (s, rho)),
;
; and c is then a genuine CONSTANT determined by matching, computed here exactly and CHECKED: the whole answer
; is gated by the differentiation certificate D(c log g) = integrand inside K (af-certify).  When p(s) is not a
; perfect square in Q (rho irrational) the construction would need Q(rho); this rung reports 'needs-extension
; honestly rather than guessing.  When p(s) = 0 (the pole sits at a branch point) it reports 'branch-pole.
;
; This closes INT dx/((x - s) sqrt(quadratic)) GENERALLY -- including the shifted-pole cases the naive formula
; got wrong -- and is the first genus where the third-kind logarithm is unconditional.  Builds on algfunc.lisp.

(import "cas/algfunc.lisp")

; perfect-square test over Q: is the rational q the square of a rational?  returns its root or 'no.
(define (at3-sqrt-q q)
  (if (< q 0) (quote no)
      (let ((n (numerator q)) (d (denominator q)))
        (let ((rn (at3-isqrt n)) (rd (at3-isqrt d)))
          (if (if (equal? rn (quote no)) #t (equal? rd (quote no))) (quote no) (/ rn rd))))))
(define (at3-isqrt n) (at3-isqrt-go n 0))
(define (at3-isqrt-go n k) (cond ((= (* k k) n) k) ((> (* k k) n) (quote no)) (else (at3-isqrt-go n (+ k 1)))))

; INT 1/((x-s) sqrt(p)) dx for p a squarefree quadratic (a rat with denominator 1), s a rational pole.
; -> (list 'log clog g)  meaning clog * log(g), g in K, certified
;  | (list 'needs-extension rho2)   (p(s) not a perfect square; answer lives over Q(sqrt(p(s))))
;  | (list 'branch-pole)            (p(s) = 0)
;  | (list 'not-genus0)             (p not degree 2)
(define (at3-logpart p s)
  (let ((pp (rat-num p)))
    (if (not (= (poly-deg pp) 2)) (list (quote not-genus0))
        (let ((ps (poly-eval pp s)))
          (if (= ps 0) (list (quote branch-pole))
              (let ((rho (at3-sqrt-q ps)))
                (if (equal? rho (quote no)) (list (quote needs-extension) ps)
                    (at3-build p pp s rho))))))))

(define (at3-build p pp s rho)
  (let ((k (/ (poly-eval (poly-deriv pp) s) (* 2 rho))))
    (let ((L (poly-add (list rho) (poly-scale k (list (- 0 s) 1)))))      ; rho + k(x-s)
      (let ((g (af-make (rat-make (poly-neg L) (list (- 0 s) 1)) (rat-make (list 1) (list (- 0 s) 1)))))
        (let ((integ (af-make (rat-zero) (rat-make (list 1) (poly-mul (list (- 0 s) 1) pp)))))
          (let ((gpovg (af-div p (af-deriv p g) g)))
            (let ((clog (rat-div (af-v integ) (af-v gpovg))))
              (if (if (<= (poly-deg (rat-num clog)) 0) (<= (poly-deg (rat-den clog)) 0) #f)
                  (if (af-certify p (af-zero) clog g integ) (list (quote log) clog g) (list (quote certificate-failed)))
                  (list (quote certificate-failed))))))))))

; certified decision wrapper
(define (at3-decides? p s) (equal? (car (at3-logpart p s)) (quote log)))
; certificate re-check (used by the golden): D(clog log g) = 1/((x-s) sqrt p)
(define (at3-verify p s)
  (let ((r (at3-logpart p s)))
    (if (equal? (car r) (quote log))
        (let ((clog (car (cdr r))) (g (car (cdr (cdr r)))))
          (af-certify p (af-zero) clog g (af-make (rat-zero) (rat-make (list 1) (poly-mul (list (- 0 s) 1) (rat-num p))))))
        #f)))
