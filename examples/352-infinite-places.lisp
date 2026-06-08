; The INFINITE PLACES of the hyperelliptic curve y^2 = q(x): how many points lie over x = infinity, whether the
; degree-2 x-cover ramifies there, and an INDEPENDENT genus by Riemann-Hurwitz -- completing the place/genus
; picture that the finite integral basis and the genus decision assume (docs/CAS.md -- summit S2).
;
; Over x = infinity the behavior is fixed by the parity of d = deg q and the leading coefficient: d odd gives one
; ramified place; d even gives two places when the leading coefficient is a perfect square, else one.  Counting
; ramification (the d finite branch points of squarefree q, plus infinity when d is odd) and applying
; Riemann-Hurwitz 2g - 2 = -4 + R yields g = (R-2)/2 = floor((d-1)/2) -- an independent confirmation of the genus.
(import "cas/infplaces.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Places at infinity of y^2 = q, ramification of the x-cover, and the Riemann-Hurwitz genus.") (newline) (newline)

(define q3 (list 1 0 0 1))         ; x^3 + 1
(define q4 (list 1 0 0 0 1))       ; x^4 + 1
(define q5 (list 1 0 0 0 0 1))     ; x^5 + 1
(define q6 (list 1 0 0 0 0 0 1))   ; x^6 + 1

(display "odd degree -- y^2 = x^3+1: one place at infinity, the cover ramifies there:") (newline)
(chk "infinity is a branch point (d = 3 odd)" (ip-infinite-ramified? q3))
(chk "exactly one place over infinity" (= (ip-num-infinite-places q3) 1))
(chk "Riemann-Hurwitz genus is 1 (elliptic)" (= (ip-genus-rh q3) 1))

(display "even degree, square leading coefficient -- y^2 = x^4+1: two unramified places at infinity:") (newline)
(chk "infinity is not a branch point (d = 4 even)" (if (ip-infinite-ramified? q4) #f #t))
(chk "two places over infinity (leading coefficient 1 is a square)" (= (ip-num-infinite-places q4) 2))
(chk "Riemann-Hurwitz genus is 1" (= (ip-genus-rh q4) 1))

(display "even degree, NON-square leading coefficient -- y^2 = 2x^4+1: a single place at infinity:") (newline)
(chk "one place over infinity (leading coefficient 2 is not a square)" (= (ip-num-infinite-places (list 1 0 0 0 2)) 1))

(display "the two genus computations agree -- Riemann-Hurwitz versus floor((deg q - 1)/2):") (newline)
(chk "agree for x^3+1, x^4+1, x^5+1, x^6+1" (if (ip-genus-agrees? q3) (if (ip-genus-agrees? q4) (if (ip-genus-agrees? q5) (ip-genus-agrees? q6) #f) #f) #f))
(chk "x^5+1 has Riemann-Hurwitz genus 2; x^6+1 also genus 2" (if (= (ip-genus-rh q5) 2) (= (ip-genus-rh q6) 2) #f))

(newline)
(display "The infinite places are now classified exactly -- count and ramification from the degree and leading") (newline)
(display "coefficient -- and the hyperelliptic genus is confirmed by an independent Riemann-Hurwitz count, agreeing") (newline)
(display "with the degree formula on every case.  The general degree > 2 integral closure remains the open summit.") (newline)
