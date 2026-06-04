; The GENUS of SUPERELLIPTIC curves y^n = f(x) for any n >= 2, generalizing the hyperelliptic (n = 2) genus to
; higher cyclic covers (docs/CAS.md -- summit S2, genus beyond degree-2-in-y).
;
; For y^n = f(x) with f squarefree of degree d, the x-map is a degree-n cyclic cover and the genus is
; g = (1/2)[(n-1)(d-1) - (gcd(n,d) - 1)].  This is confirmed INDEPENDENTLY by Riemann-Hurwitz from the
; ramification R = d(n-1) + (n - gcd(n,d)): 2g - 2 = -2n + R.  Both are exact integer arithmetic and agree on every
; case; for n = 2 they reduce to floor((d-1)/2), matching the hyperelliptic modules.
(import "cas/superelliptic.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define f3 (list 1 0 0 1))      ; degree 3
(define f4 (list 1 0 0 0 1))    ; degree 4
(define f5 (list 1 0 0 0 0 1))  ; degree 5

(display "Superelliptic genus of y^n = f(x), with an independent Riemann-Hurwitz cross-check.") (newline) (newline)

(display "at n = 2 the formula recovers the hyperelliptic genus floor((deg f - 1)/2):") (newline)
(chk "y^2 = f (deg 3) has genus 1" (= (sup-genus 2 f3) 1))
(chk "y^2 = f (deg 5) has genus 2" (= (sup-genus 2 f5) 2))
(chk "the n = 2 genus matches floor((d-1)/2) for degree 3" (sup-reduces-to-hyperelliptic? f3))
(chk "...and for degree 4" (sup-reduces-to-hyperelliptic? f4))

(display "genuine higher cyclic covers: y^3 = f and y^4 = f:") (newline)
(chk "y^3 = f (deg 4) has genus 3" (= (sup-genus 3 f4) 3))
(chk "y^3 = f (deg 5) has genus 4" (= (sup-genus 3 f5) 4))
(chk "y^3 = f (deg 3) has genus 1, like a plane cubic" (= (sup-genus 3 f3) 1))
(chk "y^4 = f (deg 3) has genus 3" (= (sup-genus 4 f3) 3))
(chk "y^4 = f (deg 5) has genus 6" (= (sup-genus 4 f5) 6))

(display "the closed formula and Riemann-Hurwitz agree across n = 2..6:") (newline)
(chk "agree for y^3 = f(deg 4)" (sup-genus-agrees? 3 f4))
(chk "agree for y^4 = f(deg 5)" (sup-genus-agrees? 4 f5))
(chk "agree for y^5 = f(deg 4)" (sup-genus-agrees? 5 f4))
(chk "agree for y^6 = f(deg 4)" (sup-genus-agrees? 6 f4))

(display "the places over infinity number gcd(n, deg f):") (newline)
(chk "y^3 = f(deg 4): one infinite place (gcd 1)" (= (sup-infinite-places 3 f4) 1))
(chk "y^4 = f(deg 4): four infinite places (gcd 4)" (= (sup-infinite-places 4 f4) 4))
(chk "y^2 = f(deg 4): two infinite places (gcd 2)" (= (sup-infinite-places 2 f4) 2))

(newline)
(display "The genus of superelliptic curves y^n = f is now computed for any n, confirmed by an independent") (newline)
(display "Riemann-Hurwitz count and consistent with the hyperelliptic case at n = 2.  The full integral closure of") (newline)
(display "these higher covers at all places remains the continuing summit.") (newline)
