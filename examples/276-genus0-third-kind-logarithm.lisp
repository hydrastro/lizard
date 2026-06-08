; RUNG 3a of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): the THIRD-KIND ALGEBRAIC LOGARITHM for the
; genus-0 radical (p a squarefree quadratic).
;
; A simple pole of the rational part of a differential on y^2 = p carries a nonzero residue (Rung 1) -- the
; integral is THIRD KIND and its antiderivative is an algebraic logarithm c log(g), g in K = Q(x)[y]/(y^2 - p).
; For genus 0 the residue divisor is always principal, so the logarithm always exists.  The previous session's
; probe showed the NAIVE form (y - sqrt(p(s)))/(x - s) is wrong off the pole-at-origin case; the correct log
; argument uses the TANGENT LINE to the curve at the point over the pole:
;
;     INT dx/((x - s) sqrt(p))  =  c * log( (y - L(x)) / (x - s) ),
;     L(x) = rho + k (x - s),  rho^2 = p(s),  k = p'(s)/(2 rho)  (the curve's slope at (s, rho)),
;
; with c a constant computed exactly and the whole answer gated by the differentiation certificate
; D(c log g) = integrand inside K.  When p(s) is not a perfect square in Q the answer lives over Q(sqrt(p(s)))
; and is reported 'needs-extension; when p(s) = 0 the pole is a branch point, reported 'branch-pole.  Nothing
; is guessed.
;
; This closes INT dx/((x - s) sqrt(quadratic)) GENERALLY -- including the shifted poles the naive formula got
; wrong -- the first genus where the third-kind logarithm is unconditional.
(import "cas/algthird.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Genus-0 third-kind logarithm INT dx/((x-s) sqrt(p)) = c log((y - L(x))/(x-s)), certified in K:") (newline) (newline)

(display "pole at the origin, p = x^2+1:") (newline)
(define r1 (at3-logpart (rat-from-poly (list 1 0 1)) 0))
(display "  INT dx/(x sqrt(x^2+1)) -> ") (display (car r1)) (display " ;  c = ") (display (car (cdr r1))) (newline)
(chk "INT dx/(x sqrt(x^2+1)) = log((sqrt(x^2+1)-1)/x), certified" (at3-verify (rat-from-poly (list 1 0 1)) 0))

(display "SHIFTED pole (the case the naive formula failed), p = x^2+3, s = 1:") (newline)
(define r2 (at3-logpart (rat-from-poly (list 3 0 1)) 1))
(display "  INT dx/((x-1) sqrt(x^2+3)) -> ") (display (car r2)) (display " ;  c = ") (display (car (cdr r2))) (newline)
(chk "INT dx/((x-1) sqrt(x^2+3)) certified via the tangent-line construction" (at3-verify (rat-from-poly (list 3 0 1)) 1))

(display "general shifted pole, p = x^2-4x+5, s = 2:") (newline)
(chk "INT dx/((x-2) sqrt(x^2-4x+5)) certified" (at3-verify (rat-from-poly (list 5 -4 1)) 2))

(newline)
(display "soundness (no guessed answers):") (newline)
(define rext (at3-logpart (rat-from-poly (list 2 0 1)) 0))
(display "  p = x^2+2, s = 0 (p(0)=2 not a perfect square) -> ") (display (car rext)) (newline)
(chk "non-square p(s) honestly reported needs-extension, not a false logarithm" (equal? (car rext) (quote needs-extension)))
(define rb (at3-logpart (rat-from-poly (list -1 0 1)) 1))
(display "  p = x^2-1, s = 1 (p(1)=0, pole at a branch point) -> ") (display (car rb)) (newline)
(chk "branch pole honestly reported branch-pole" (equal? (car rb) (quote branch-pole)))

(newline)
(display "RUNG 3a reached: the genus-0 third-kind algebraic logarithm, correct for shifted poles, certified, sound.") (newline)
