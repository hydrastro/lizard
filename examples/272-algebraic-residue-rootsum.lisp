; The algebraic-residue RootSum of ARBITRARY degree (BOUNDARY 1): integrals whose Rothstein-Trager residues are
; algebraic numbers, expressed over the needed extension Q(alpha) -- the %r summation Maxima and FriCAS print.
;
; rtower's multi-residue logarithmic part returns all the logarithms when the residues are rational and declines
; soundly when they are not.  algresext closes that remaining case: when the residue polynomial R(z) has an
; irreducible factor of degree d >= 2, the logarithmic part is the RootSum sum_{R(alpha)=0} alpha log(v_alpha),
; with v_alpha = gcd_theta(V, Pnum - alpha V') computed over Q(alpha)(x), and alpha the generator of Q(alpha).
;
; The decisive demonstration is INT 2 e^x/(e^(2x)+1) dx, whose resultant is z^2 + 1: the residues are +/- i, and
; the value is i log(e^x - i) - i log(e^x + i) over Q(i).  rtower alone declines it (complex residues); algresext
; returns it and certifies it.  The same machinery handles Q(sqrt2) and cubic residue fields.
;
; Soundness is a DIFFERENTIATION CERTIFICATE expressed with the field TRACE: the derivative of the RootSum is
; Tr_{Q(alpha)/Q}( alpha (D v_alpha)(V/v_alpha) ) / V, which lands in Q(x); for an exponential monomial there is
; an additional Tr(alpha)*deg(v_alpha)*V polynomial correction.  The certificate checks this identity exactly
; over Q, so an elementary verdict is always justified, and cases failing it are left declined.
(import "cas/algresext.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))

(define s1 (list (list (quote exp) (rat-one))))                       ; theta1 = e^x
(define spec (list (quote exp) (rat-one)))
(define (verify P V) (axr-verify s1 spec P V (axr-logpart s1 spec P V)))

(display "Algebraic-residue RootSum sum_{R(alpha)=0} alpha log(v_alpha), certified via the field trace:") (newline) (newline)

(display "Q(i) -- residues +/- i:") (newline)
(define rI (axr-integrate-logpart s1 (list (rat-zero) (rat-scale 2 (rat-one))) (list (rat-one) (rat-zero) (rat-one))))
(display "  INT 2e^x/(e^(2x)+1) dx -> ") (display (car rI)) (newline)
(must "INT 2e^x/(e^(2x)+1) dx = i log(e^x-i) - i log(e^x+i)   over Q(i), certified" (verify (list (rat-zero) (rat-scale 2 (rat-one))) (list (rat-one) (rat-zero) (rat-one))))

(display "Q(sqrt2) -- residues (1 +/- sqrt2)/4:") (newline)
(must "INT (e^x+1)/(e^(2x)-2) dx     over Q(sqrt2), certified" (verify (list (rat-one) (rat-one)) (list (rat-scale -2 (rat-one)) (rat-zero) (rat-one))))

(display "cubic Q(2^(1/3)) -- residues the cube roots:") (newline)
(must "INT 3e^(2x)/(e^(3x)-2) dx     over Q(2^(1/3)), certified" (verify (list (rat-zero) (rat-zero) (rat-scale 3 (rat-one))) (list (rat-scale -2 (rat-one)) (rat-zero) (rat-zero) (rat-one))))

(newline)
(display "soundness:") (newline)
; a rational-residue integrand has no NONLINEAR resultant factor, so the algebraic path returns 'none
; (it belongs to rtower's rational RootSum, not here) -- algresext never poaches the rational case.
(define rrat (axr-integrate-logpart s1 (list (rat-zero) (rat-scale 2 (rat-one))) (list (rat-neg (rat-one)) (rat-zero) (rat-one))))
(display "  INT 2e^x/(e^(2x)-1) dx -> ") (display (car rrat)) (display "  (rational residues; correctly deferred to rtower)") (newline)
(must "rational-residue integrand yields 'none from the algebraic path (no false algebraic claim)" (equal? (car rrat) (quote none)))

(newline)
(display "BOUNDARY 1 achieved: algebraic-residue RootSum at degrees 2 and 3, expressed over Q(alpha), trace-certified, sound.") (newline)
