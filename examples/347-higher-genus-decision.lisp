; The general higher-genus DECISION for INT P(x)/sqrt(q(x)) dx with q squarefree of any degree >= 7 (genus >= 3):
; decide elementarity, certify the algebraic antiderivative when it exists, otherwise report a genuine
; higher-genus hyperelliptic integral, non-elementary by exact reduction (docs/CAS.md -- frontier b: genus >= 3).
;
; The Hermite-style radical reduction is degree-general; the curve y^2 = q has genus floor((deg q - 1)/2), so a
; nonzero reduced remainder is a first/second-kind higher-genus differential and the integral is non-elementary.
; This completes the radical-integration tower across ALL genera (elliptic genus 1, hyperelliptic genus 2, and
; now genus 3 and beyond), reusing one reducer and one certificate.
(import "cas/hypergenus.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define q7 (list 1 0 0 0 0 0 0 1))      ; x^7 + 1, genus 3
(define q8 (list 1 0 0 0 0 0 0 0 1))    ; x^8 + 1, genus 3
(define q9 (list 1 0 0 0 0 0 0 0 0 1))  ; x^9 + 1, genus 4

(display "Higher-genus radical integrals INT P(x)/sqrt(q), q squarefree of degree >= 7.") (newline) (newline)

(display "the genus of y^2 = q is floor((deg q - 1)/2):") (newline)
(chk "genus(x^7+1) = genus(x^8+1) = 3, genus(x^9+1) = 4" (if (= (hg-genus q7) 3) (if (= (hg-genus q8) 3) (= (hg-genus q9) 4) #f) #f))

(display "the ELEMENTARY case -- numerator matches the derivative pattern:") (newline)
(display "INT (7x^6/2)/sqrt(x^7+1) dx = sqrt(x^7+1)  [genus 3, since (x^7+1)' = 7x^6]:") (newline)
(define r1 (hg-integrate (list 0 0 0 0 0 0 (/ 7 2)) q7))
(chk "INT (7x^6/2)/sqrt(x^7+1) elementary, certified in K" (if (equal? (car r1) (quote elementary)) (hg-certify (list 0 0 0 0 0 0 (/ 7 2)) q7 r1) #f))
(display "INT 4x^7/sqrt(x^8+1) dx = sqrt(x^8+1)  [octic, genus 3]:") (newline)
(chk "INT 4x^7/sqrt(x^8+1) elementary, certified" (if (equal? (car (hg-integrate (list 0 0 0 0 0 0 0 4) q8)) (quote elementary)) (hg-certify (list 0 0 0 0 0 0 0 4) q8 (hg-integrate (list 0 0 0 0 0 0 0 4) q8)) #f))

(display "the GENUINE higher-genus integrals -- proven non-elementary, with the genus reported:") (newline)
(define rn (hg-integrate (list 1) q7))
(chk "INT 1/sqrt(x^7+1) non-elementary, genus 3" (if (equal? (car rn) (quote non-elementary)) (= (car (cdr (car (cdr rn)))) 3) #f))
(chk "INT x/sqrt(x^7+1) non-elementary" (equal? (car (hg-integrate (list 0 1) q7)) (quote non-elementary)))
(chk "INT 1/sqrt(x^9+1) non-elementary, genus 4" (if (equal? (car (hg-integrate (list 1) q9)) (quote non-elementary)) (= (car (cdr (car (cdr (hg-integrate (list 1) q9))))) 4) #f))

(display "integrate as far as possible: INT sqrt(x^7+1) = (2/9) x sqrt(x^7+1) + (7/9) INT 1/sqrt(x^7+1):") (newline)
(chk "INT sqrt(x^7+1) splits into elementary part + named higher-genus remainder" (equal? (car (hg-split (list 1 0 0 0 0 0 0 1) q7)) (quote split)))

(newline)
(display "The radical-integration tower is now complete across all genera: genus 1 (elliptic), genus 2") (newline)
(display "(hyperelliptic), and genus 3 and beyond -- the rare elementary case certified inside the function field,") (newline)
(display "the genuine higher-genus integrals proven non-elementary by one degree-general reduction.  Full third-kind") (newline)
(display "logarithmic parts on these curves and Trager integral bases remain the continuing summit.") (newline)
