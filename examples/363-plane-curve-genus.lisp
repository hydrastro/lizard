; The GENUS of a plane algebraic curve by the genus-degree (Plucker) formula, with corrections for ordinary
; singular points -- a complementary view to the superelliptic cyclic-cover formula, agreeing where both apply
; (docs/CAS.md -- summit S2, genus of general curves).
;
; A smooth plane curve of degree d has genus (d-1)(d-2)/2.  Each ordinary m-fold point lowers the geometric genus
; by the delta-invariant m(m-1)/2 (a node m=2 by 1, a triple point m=3 by 3).  Exact integer arithmetic; the
; smooth plane cubic and the superelliptic y^2 = cubic give the same genus, which is the cross-check.
(import "cas/planecurve.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Genus of a plane curve by the genus-degree formula, with singularity corrections.") (newline) (newline)

(display "smooth plane curves -- the classical genera (d-1)(d-2)/2:") (newline)
(chk "a smooth conic (degree 2) has genus 0" (= (pc-smooth-genus 2) 0))
(chk "a smooth cubic (degree 3) has genus 1 -- an elliptic curve" (= (pc-smooth-genus 3) 1))
(chk "a smooth quartic (degree 4) has genus 3" (= (pc-smooth-genus 4) 3))
(chk "a smooth quintic (degree 5) has genus 6" (= (pc-smooth-genus 5) 6))
(chk "a smooth sextic (degree 6) has genus 10" (= (pc-smooth-genus 6) 10))

(display "singularities lower the genus by their delta-invariants:") (newline)
(chk "a node (double point) has delta-invariant 1" (= (pc-delta 2) 1))
(chk "an ordinary triple point has delta-invariant 3" (= (pc-delta 3) 3))
(chk "a nodal cubic (one node) drops to genus 0 -- it is rational" (= (pc-genus 3 (list 2)) 0))
(chk "and is detected as rational" (pc-is-rational? 3 (list 2)))
(chk "a quartic with three nodes has genus 0" (= (pc-genus 4 (list 2 2 2)) 0))
(chk "a quartic with one node has genus 2" (= (pc-genus 4 (list 2)) 2))

(display "cross-check: the smooth plane cubic and the superelliptic y^2 = cubic give the same genus:") (newline)
(chk "the two independent genus computations agree (both 1)" (pc-agrees-superelliptic-cubic?))

(newline)
(display "The genus-degree formula gives the genus of plane curves -- smooth or with ordinary singularities -- as a") (newline)
(display "second, independent computation that agrees with the cyclic-cover formula where they overlap.  Resolving") (newline)
(display "arbitrary singularities and the full integral closure at every place remain the open frontier.") (newline)
