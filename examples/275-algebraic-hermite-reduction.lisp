; RUNG 2 of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): HERMITE REDUCTION for a rational-function
; numerator over sqrt(p).
;
; hyperell integrates a POLYNOMIAL numerator P(x)/sqrt(p); Rung 1 located the residue obstruction in the SIMPLE
; poles of the rational part.  This rung removes the HIGHER-ORDER (second-kind) poles: for a differential
; f = w(x) y with w = A/D a proper rational function over Q(x) (y = sqrt(p)), it produces
;
;     INT w y dx = G y + INT wbar y dx,
;
; with G in Q(x) and wbar having at most a SIMPLE pole at every non-branch place.  The reduction is the
; algebraic Hermite method (Bronstein 5.3) carried out over Q via the squarefree factorization of D -- so no
; irrational roots are ever introduced -- and every step is checked by the differentiation certificate
; D(G y) + wbar y = w y inside K = Q(x)[y]/(y^2 - p).  Repeated factors that meet a branch point (a common root
; with p) are left for the ramified-place machinery of a later rung, never mis-reduced.
;
; This is the piece that turns "integrate P(x)/sqrt(p)" into "integrate A(x)/B(x) over sqrt(p)" for the
; second-kind part, and it feeds the third-kind logarithm rung the clean simple-pole remainder.
(import "cas/algherm.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))
(define (ppow b n) (if (= n 0) (list 1) (poly-mul b (ppow b (- n 1)))))
(define (simple-at? w s) (<= (poly-deg (poly-gcd (rat-den w) (ppow (list (- 0 s) 1) 2))) 1))

(display "Algebraic Hermite reduction INT w(x) sqrt(p) dx = G sqrt(p) + INT wbar sqrt(p) dx, certified in K:") (newline) (newline)

(define pq (rat-from-poly (list 1 0 1)))     ; sqrt(x^2+1)  (genus 0)
(define pe (rat-from-poly (list 1 0 0 1)))   ; sqrt(x^3+1)  (genus 1, elliptic)

(display "double pole, p = x^2+1:") (newline)
(define wd (rat-make (list 1) (ppow (list -2 1) 2)))   ; 1/(x-2)^2
(define rd (ah-reduce wd pq))
(display "  INT (1/(x-2)^2) sqrt(x^2+1) dx -> G = ") (display (car (cdr rd))) (display " ; wbar pole at x=2 now simple") (newline)
(chk "double pole reduces and CERTIFIES (D(G y)+wbar y = w y)" (ah-verify wd pq))
(chk "  remainder wbar has only a simple pole at x=2" (simple-at? (car (cdr (cdr rd))) 2))

(display "triple pole, p = x^2+1:") (newline)
(chk "INT (1/(x-2)^3) sqrt(x^2+1) dx reduces and certifies" (ah-verify (rat-make (list 1) (ppow (list -2 1) 3)) pq))

(display "two distinct double poles, p = x^2+1:") (newline)
(define wtwo (rat-make (list 1) (poly-mul (ppow (list -2 1) 2) (ppow (list 1 1) 2))))
(chk "INT 1/((x-2)^2 (x+1)^2) sqrt(x^2+1) dx reduces and certifies" (ah-verify wtwo pq))
(define rtwo (ah-reduce wtwo pq))
(chk "  wbar simple at x=2 and x=-1" (if (simple-at? (car (cdr (cdr rtwo))) 2) (simple-at? (car (cdr (cdr rtwo))) -1) #f))

(display "elliptic curve p = x^3+1, double pole at a non-branch point:") (newline)
(chk "INT (1/(x-2)^2) sqrt(x^3+1) dx reduces and certifies" (ah-verify (rat-make (list 1) (ppow (list -2 1) 2)) pe))

(display "no-op cases (already reduced):") (newline)
(define rs (ah-reduce (rat-make (list 1) (list -2 1)) pq))
(chk "simple pole -> G = 0, certifies" (rat-zero? (car (cdr rs))))
(chk "polynomial numerator -> reduces trivially, certifies" (ah-verify (rat-from-poly (list 0 0 1)) pq))

(newline)
(display "RUNG 2 reached: algebraic Hermite reduction removes higher-order second-kind poles over Q, certified; simple-pole remainder ready for the third-kind logarithm.") (newline)
